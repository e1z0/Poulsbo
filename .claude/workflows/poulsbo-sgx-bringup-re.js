export const meta = {
  name: 'poulsbo-sgx-bringup-re',
  description: 'Reverse-engineer the real SGX535 command-submission sequence from EMGD/openpvrsgx and produce a grounded bring-up plan + first safe hardware increment for the Poulsbo driver',
  phases: [
    { title: 'Extract', detail: 'parallel RE readers, one per source of ground truth' },
    { title: 'Synthesize', detail: 'merge into real submission sequence + gap analysis + milestone roadmap' },
    { title: 'Draft', detail: 'first safe read-only hardware increment + adversarial correctness review' },
  ],
}

const REPO = '/Projects/Poulsbo'

const EXTRACT_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  required: ['source', 'summary', 'hardware_sequence', 'registers', 'firmware_context', 'key_files', 'gaps_vs_repo'],
  properties: {
    source: { type: 'string', description: 'which source this covers' },
    summary: { type: 'string', description: '3-6 sentence plain-English summary of what this source reveals about real SGX535 command submission' },
    hardware_sequence: {
      type: 'array', description: 'ordered real steps the hardware requires to execute one command (init -> submit -> completion)',
      items: { type: 'object', additionalProperties: false, required: ['step', 'detail'], properties: {
        step: { type: 'string' }, detail: { type: 'string', description: 'what registers/memory/firmware are involved, with file:line evidence' } } }
    },
    registers: {
      type: 'array', description: 'concrete SGX/EUR MMIO registers involved with offsets if known',
      items: { type: 'object', additionalProperties: false, required: ['name', 'purpose'], properties: {
        name: { type: 'string' }, offset: { type: 'string' }, purpose: { type: 'string' } } }
    },
    firmware_context: { type: 'string', description: 'Does real submission require SGX microkernel firmware upload / MMU page-directory / per-context setup? Explain what and whether it is available in the repo assets.' },
    key_files: { type: 'array', items: { type: 'string' }, description: 'file:line references worth citing' },
    gaps_vs_repo: { type: 'string', description: 'what the real hardware needs that the repo software model does NOT do' },
  },
}

phase('Extract')
const SOURCES = [
  { key: 'emgd-sgxkick',
    prompt: `You are reverse-engineering the REAL PowerVR SGX535 command-submission path from Intel's EMGD source in ${REPO}/research/external/intel-binaries-linux/drm/pvr/services4/srvkm/devices/sgx/ (read sgxkick.c, sgxinit.c, sgxutils.c and their headers, plus any sgxconfig/sgxdefs headers you find under research/external/intel-binaries-linux). Extract the EXACT sequence to submit ONE render/TA command to SGX and get completion: how the CCB (circular command buffer) is written, how the kick/doorbell happens (which register write actually starts the GPU), what EUR_CR_* registers are touched, how completion is signalled (interrupt status registers), and whether the SGX microkernel firmware must be uploaded first and an MMU page directory set up. Cite file:line. Be concrete about register names/offsets.` },
  { key: 'openpvrsgx',
    prompt: `Reverse-engineer the modern OPEN SGX submission + bringup path from the openpvrsgx tree in ${REPO}/research/external/linux_openpvrsgx. Find the pvrsgx/SGX driver sources (search under drivers/ and Letux for sgx, pvr, omap sgx, eurasia). Extract: SGX core init sequence, microkernel firmware load, clock/power enable for SGX on an SoC, MMU/page-directory setup, the kick register write, and interrupt/completion handling. Focus on anything applicable to SGX535 / EUR revisions. Cite file:line and concrete register names/offsets.` },
  { key: 'repo-real-hw',
    prompt: `In ${REPO}/build/devnull-vaio-gma500-src/psb_drv.c (27k lines, use sudo to read if needed) and the surrounding gma500 sources, find the ~9 REAL MMIO accesses (PSB_RSGX32 / PSB_WSGX32) and the 2D imageblit + SGX IRQ instrumentation that ACTUALLY runs on the target (dmesg shows "2D imageblit submit" and "SGX irq: TWOD_COMPLETE"). Determine exactly which SGX registers are already read/written today, how the 2D blit is submitted to real hardware, and how the real SGX interrupt status is decoded. This is the proven-working hardware access we can build on. Cite file:line and register names/offsets. Also note how the driver maps SGX MMIO (which BAR, which base like dev_priv->sgx_reg).` },
  { key: 'repo-software-model',
    prompt: `Analyze the in-kernel SGX-lab SOFTWARE MODEL in ${REPO}/build/devnull-vaio-gma500-src/psb_drv.c (use sudo to read) and the ABI in ${REPO}/build/devnull-vaio-gma500-src/psb_drm_lab.h. The submit pipeline (validate/prepare/arm/emit/launch/issue/transition/dispatch + report/result/manifest/descriptor/plan) threads wrapper structs but does almost no real MMIO. Map the "simulation boundary": which stages compute/track state in software vs which (if any) touch hardware, and identify the single seam where a REAL ccb-kick would need to be inserted to replace simulation with hardware. Describe the CCB record format the ABI already models (PSB_SGX_LAB_CCB_* offsets, PDS/USE/TARGET/STATUS slots) and how close it is to the real SGX CCB. Cite file:line.` },
  { key: 'hw-ground-truth',
    prompt: `Establish ground truth about THIS silicon (Intel US15W Poulsbo, PCI 8086:8108, SGX535) from ${REPO}/data/psb_registers.csv, ${REPO}/docs/05-hardware-notes.md, ${REPO}/docs/09-first-hardware-session.md, and anything under ${REPO}/target-captures/. Extract the documented SGX register map, MMIO windows/BARs, memory (GTT) layout, and any real captures already taken from the hardware. List which SGX registers are known and their offsets, and what a SAFE read-only register dump to verify hardware identity (SGX core ID / revision / config) would read. Cite file:line.` },
  { key: 'userspace-stack-gap',
    prompt: `Read ${REPO}/docs/03-mesa-driver-design.md, ${REPO}/docs/04-xorg-path.md, ${REPO}/docs/01-reverse-engineering-plan.md and skim the workloads in ${REPO}/workloads (gles2_*.c, x11_*.c, drm_probe.c). Summarize the intended userspace stack (Mesa Gallium GLES2 + Xorg modesetting/glamor), what exists today (appears to be only probes + the lab ioctl clients, no Mesa driver), and the ordered dependency chain from "kernel can submit one real command" up to "GLES2 triangle on screen". Keep it concise; identify the true critical path and what is premature.` },
]

const extracts = await parallel(SOURCES.map(s => () =>
  agent(s.prompt, { label: `extract:${s.key}`, phase: 'Extract', schema: EXTRACT_SCHEMA, effort: 'high' })
))
const valid = extracts.filter(Boolean)
log(`Extract phase done: ${valid.length}/${SOURCES.length} sources analyzed`)

phase('Synthesize')
const SYNTH_SCHEMA = {
  type: 'object', additionalProperties: false,
  required: ['verdict', 'real_submission_sequence', 'gap_analysis', 'roadmap', 'first_increment_spec', 'risks'],
  properties: {
    verdict: { type: 'string', description: 'honest 4-8 sentence assessment: what the repo really is today, what works, what is simulation, and what the shortest real path forward is' },
    real_submission_sequence: { type: 'array', items: { type: 'object', additionalProperties: false, required: ['phase','detail'], properties: { phase: { type: 'string' }, detail: { type: 'string' } } },
      description: 'the definitive real SGX535 sequence to execute one command on THIS hardware, reconciled across EMGD + openpvrsgx + repo real-hw' },
    gap_analysis: { type: 'array', items: { type: 'object', additionalProperties: false, required: ['capability','status','note'], properties: { capability: { type: 'string' }, status: { type: 'string', enum: ['works-on-hw','simulated-only','missing','not-started'] }, note: { type: 'string' } } } },
    roadmap: { type: 'array', items: { type: 'object', additionalProperties: false, required: ['milestone','goal','concrete_tasks','proves'], properties: { milestone: { type: 'string' }, goal: { type: 'string' }, concrete_tasks: { type: 'array', items: { type: 'string' } }, proves: { type: 'string' } } },
      description: 'ordered M0..Mn milestones from safe hardware ground-truth to first real 3D command to GLES2 triangle' },
    first_increment_spec: { type: 'string', description: 'precise spec of the FIRST safe, read-only hardware increment: what code to add, where, which SGX registers to dump, expected values, how to build for kernel 5.4.0-150 i686 and run on the Vaio, and the pass/fail criterion. Must be read-only (no writes that could hang/panic the GPU).' },
    risks: { type: 'array', items: { type: 'string' } },
  },
}
const synthPrompt = `You are the lead reverse-engineer synthesizing findings for the Poulsbo/GMA500 SGX535 driver bring-up. Here are the extraction results as JSON:\n\n${JSON.stringify(valid, null, 2)}\n\nReconcile them into: (1) an honest verdict on the current repo state, (2) THE definitive real hardware command-submission sequence for SGX535 on Intel US15W Poulsbo, (3) a gap analysis (works-on-hw vs simulated-only vs missing), (4) an ordered milestone roadmap M0..Mn whose FIRST milestone is a SAFE read-only hardware-identity verification and whose subsequent milestones progress to a first real TA/render command and then toward a GLES2 triangle, and (5) a precise spec for the first safe read-only increment. The target is real hardware that has panicked before — the first increment MUST be read-only. Be concrete and cite the register/file evidence from the extracts.`
const synth = await agent(synthPrompt, { label: 'synthesize', phase: 'Synthesize', schema: SYNTH_SCHEMA, effort: 'high' })

phase('Draft')
const DRAFT_SCHEMA = {
  type: 'object', additionalProperties: false,
  required: ['artifact_kind', 'file_target', 'code', 'build_steps', 'run_steps', 'expected_output', 'pass_fail'],
  properties: {
    artifact_kind: { type: 'string', description: 'e.g. kernel-patch adding read-only SGX register dump, or userspace query-ioctl extension' },
    file_target: { type: 'string', description: 'exact file(s) and function(s) to touch' },
    code: { type: 'string', description: 'the actual code / unified diff, ready to apply. Read-only register access only.' },
    build_steps: { type: 'string' },
    run_steps: { type: 'string', description: 'how to deploy and run on root@10.1.11.218 (kernel 5.4.0-150 i686)' },
    expected_output: { type: 'string' },
    pass_fail: { type: 'string' },
  },
}
const draftPrompt = `Using this synthesis:\n\n${JSON.stringify(synth, null, 2)}\n\nProduce the FIRST safe, read-only hardware increment as concrete, applyable code. It must read SGX identity/status registers (e.g. SGX core ID/revision/config and IRQ status) via the driver's existing MMIO mapping and log them, WITHOUT any register write that could hang or panic the GPU. Prefer the least invasive form (extend the existing read-only query path or add a dump at module probe). Reference the real MMIO access pattern already in ${REPO}/build/devnull-vaio-gma500-src/psb_drv.c (PSB_RSGX32) so it compiles against gma500 on kernel 5.4.0-150 i686. Give the exact diff, build steps, deploy+run steps for root@10.1.11.218, expected register values (cite EMGD/openpvrsgx expectations for SGX535), and a clear pass/fail criterion.`
const draft = await agent(draftPrompt, { label: 'draft:first-increment', phase: 'Draft', schema: DRAFT_SCHEMA, effort: 'high' })

const reviewPrompt = `Adversarially review this proposed first hardware increment for the Poulsbo SGX bring-up. The target Vaio has panicked before, so safety is paramount.\n\nSYNTH:\n${JSON.stringify(synth.first_increment_spec)}\n\nDRAFT:\n${JSON.stringify(draft, null, 2)}\n\nCheck: (1) Is EVERY hardware access read-only? Flag any write to SGX/EUR registers. (2) Are the register offsets consistent with SGX535 / the repo's real PSB_RSGX32 usage and data/psb_registers.csv? Read ${REPO}/build/devnull-vaio-gma500-src/psb_drv.c and ${REPO}/data/psb_registers.csv to verify. (3) Could reading any of these registers while the GPU is powered-gated fault? Does SGX need to be clock/power-enabled before its registers are readable, and does the driver guarantee that at the chosen injection point? (4) Will it compile against gma500 on 5.4.0-150 i686? Return a clear verdict.`
const REVIEW_SCHEMA = { type: 'object', additionalProperties: false, required: ['safe_to_run','findings','verdict'], properties: {
  safe_to_run: { type: 'boolean' },
  findings: { type: 'array', items: { type: 'object', additionalProperties: false, required: ['severity','issue','fix'], properties: { severity: { type: 'string', enum: ['blocker','major','minor','nit'] }, issue: { type: 'string' }, fix: { type: 'string' } } } },
  verdict: { type: 'string' } } }
const review = await agent(reviewPrompt, { label: 'review:first-increment', phase: 'Draft', schema: REVIEW_SCHEMA, effort: 'high' })

return { synth, draft, review, sources_analyzed: valid.map(v => v.source) }