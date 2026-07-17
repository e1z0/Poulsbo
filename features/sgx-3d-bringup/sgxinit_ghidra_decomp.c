===== SGXInit @ 0x116c0 =====

/* WARNING: Function: __i686.get_pc_thunk.bx replaced with injection: get_pc_thunk_bx */

int SGXInit(undefined4 param_1,int param_2)

{
  int **ppiVar1;
  int *piVar2;
  int *piVar3;
  int *piVar4;
  int *piVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  uint **ppuVar11;
  bool bVar12;
  int iVar13;
  int **ppiVar14;
  int iVar15;
  int iVar16;
  uint uVar17;
  int *piVar18;
  uint *puVar19;
  uint uVar20;
  uint uVar21;
  uint uVar22;
  uint uVar23;
  uint *local_420;
  uint *local_41c;
  uint *local_418;
  uint *local_414;
  uint *local_410;
  int local_40c;
  uint local_408 [160];
  undefined local_188 [336];
  int local_38;
  uint *local_34;
  uint *local_30;
  uint *local_2c;
  uint *local_28;
  uint *local_24;
  undefined4 local_20 [3];
  undefined4 uStack_14;
  
  uStack_14 = 0x116d1;
  iVar13 = PVRSRVAcquireDeviceData(param_1,*(undefined4 *)(param_2 + 8),local_188,0);
  if ((iVar13 != 0) || (iVar13 = SGXGetInfoForSrvInit(local_188,&local_40c), iVar13 != 0)) {
    return iVar13;
  }
  PVRSRVMemSet(&local_38,0,0x18);
  local_38 = local_40c;
  local_420 = local_34;
  local_414 = local_30;
  local_410 = local_2c;
  local_418 = local_28;
  local_41c = local_24;
  puVar19 = local_408;
  iVar13 = 0;
LAB_00011780:
  do {
    switch(*puVar19 & 0xffffff) {
    case 2:
      local_420 = local_408 + iVar13 * 5;
      break;
    case 3:
      goto code_r0x00011988;
    case 4:
      local_410 = local_408 + iVar13 * 5;
      break;
    case 5:
      local_418 = local_408 + iVar13 * 5;
      break;
    case 6:
      local_41c = local_408 + iVar13 * 5;
    }
    puVar19 = puVar19 + 5;
    iVar13 = iVar13 + 1;
  } while (iVar13 != 0x20);
  goto code_r0x000117b0;
code_r0x00011988:
  iVar15 = iVar13 + 1;
  local_414 = local_408 + iVar13 * 5;
  puVar19 = puVar19 + 5;
  iVar13 = iVar15;
  if (iVar15 == 0x20) {
code_r0x000117b0:
    local_34 = local_420;
    local_30 = local_414;
    local_2c = local_410;
    local_28 = local_418;
    local_24 = local_41c;
    ppiVar14 = (int **)PVRSRVAllocUserModeMem(0x7a0);
    if (ppiVar14 == (int **)0x0) {
      return 2;
    }
    ppiVar14[8] = (int *)0x68018;
    ppiVar14[9] = (int *)0xdc;
    ppiVar14[10] = (int *)0x64;
    ppiVar14[0xb] = (int *)0x230;
    ppiVar14[0xc] = (int *)0x198;
    ppiVar14[0xd] = (int *)0x408;
    ppiVar14[0xe] = (int *)0x64;
    ppiVar14[0xf] = (int *)0x88;
    ppiVar14[0x10] = (int *)0x40;
    ppiVar14[0x11] = (int *)0x3c;
    ppiVar14[0x12] = (int *)0x140;
    ppiVar14[0x13] = (int *)0x3c;
    ppiVar14[0x14] = (int *)0x8c;
    ppiVar14[0x15] = (int *)0x30;
    ppiVar14[0x16] = (int *)0x34;
    ppiVar14[0x17] = (int *)0x10;
    ppiVar14[0x2a] = (int *)local_24[2];
    piVar18 = (int *)local_2c[2];
    ppiVar14[0x2f] = (int *)0x2;
    ppiVar14[0x2b] = piVar18;
    ppiVar14[0x30] = (int *)0xb10;
    ppiVar14[0x31] = (int *)0xb1c;
    ppiVar14[0x32] = (int *)0x111111;
    ppiVar14[0x34] = (int *)0x4;
    ppiVar14[0x35] = (int *)0x0;
    iVar13 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x4f,0x1000,0x1000,ppiVar14);
    if (iVar13 == 0) {
      ppiVar14[1] = (int *)**ppiVar14;
      iVar13 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x4f,8,0x1000,ppiVar14 + 3);
      if (iVar13 == 0) {
        ppiVar14[4] = (int *)*ppiVar14[3];
        iVar13 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x4f,4,0x10,ppiVar14 + 2);
        if (((iVar13 == 0) &&
            (iVar13 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x2004f,0xec,0x20,ppiVar14 + 5),
            iVar13 == 0)) &&
           (iVar13 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x2004f,0x34,0x20,ppiVar14 + 6),
           iVar13 == 0)) {
          ppiVar1 = (int **)ppiVar14[5];
          PVRSRVMemSet(*ppiVar1,0,0xec);
          piVar18 = *ppiVar1;
          *piVar18 = ppiVar14[5][2];
          piVar18[1] = ppiVar14[6][2];
          iVar13 = ppiVar14[3][2];
          piVar18[5] = 0;
          piVar18[2] = iVar13;
          piVar18[0x18] = (*ppiVar14)[2];
          piVar18[0x19] = local_38;
          piVar2 = ppiVar14[6];
          PVRSRVMemSet(*piVar2,0,0x34);
          iVar13 = *piVar2;
          iVar15 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x2004f,0x5c,0x20,ppiVar14 + 7);
          if (iVar15 == 0) {
            PVRSRVMemSet(*ppiVar14[7],0,0x5c);
            iVar15 = PVRSRVAllocDeviceMem(local_188,local_30[1],0x4b,0x3810,0x20,ppiVar14 + 0x18);
            if (iVar15 == 0) {
              PVRSRVMemSet(*ppiVar14[0x18],0,ppiVar14[0x18][6]);
              piVar18[0x37] = ppiVar14[0x18][2];
              SGXInitGetAppHint(0,"HWPerfFlags",local_20);
              *(undefined4 *)(iVar13 + 0x2c) = local_20[0];
              ppiVar14[0x23] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICKTA_00014408 >> 3);
              ppiVar14[0x24] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICKTRANSFER_000143c0 >> 3);
              ppiVar14[0x25] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICK2D_000143ac >> 3);
              ppiVar14[0x26] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICKPOWER_000143b0 >> 3);
              ppiVar14[0x27] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICKCLEANUP_000143f0 >> 3);
              ppiVar14[0x28] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICKGETMISCINFO_00014394 >> 3);
              ppiVar14[0x29] =
                   (int *)(*(uint *)PTR_ui32uKernelProgramOffsetHOSTKICKPROCESS_QUEUES_000143cc >> 3
                          );
              uVar17 = local_34[1];
              iVar13 = FUN_00011570(ppiVar14 + 0x19,
                                    *(undefined4 *)PTR_pbuKernelProgramBlocks_00014390,
                                    *(int *)PTR_ui32uKernelProgramSizes_000143c8 + 8U & 0xfffffff0);
              if (iVar13 == 0) {
                piVar18 = ppiVar14[0x19];
                iVar13 = FUN_00011570(ppiVar14 + 0x1a,
                                      *(undefined4 *)PTR_pbuKernelTAProgramBlocks_000143e0,
                                      *(int *)PTR_ui32uKernelTAProgramSizes_0001440c + 8U &
                                      0xfffffff0);
                if (iVar13 == 0) {
                  piVar2 = ppiVar14[0x1a];
                  iVar13 = FUN_00011570(ppiVar14 + 0x1b,PTR_pbuKernelSPMProgram_000143a4,0x47a0);
                  if (iVar13 == 0) {
                    piVar3 = ppiVar14[0x1b];
                    iVar13 = FUN_00011570(ppiVar14 + 0x1c,
                                          *(undefined4 *)PTR_pbuKernel3DProgramBlocks_000143d4,
                                          *(int *)PTR_ui32uKernel3DProgramSizes_000143a0 + 8U &
                                          0xfffffff0);
                    if (iVar13 == 0) {
                      piVar4 = ppiVar14[0x1c];
                      iVar13 = FUN_00011570(ppiVar14 + 0x1d,PTR_pbuKernel2DProgram_000143dc,0x39a0);
                      if (iVar13 == 0) {
                        piVar5 = ppiVar14[0x1d];
                        iVar13 = PVRSRVAllocDeviceMem(local_188,uVar17,9,0x38,0x10,ppiVar14 + 0x1e);
                        if (iVar13 == 0) {
                          puVar19 = (uint *)*ppiVar14[0x1e];
                          PVRSRVMemCopy(puVar19,&DAT_000131a0,0x38);
                          uVar20 = (*(int *)PTR_ui32uKernelProgramOffsetINITUKERNELPRIMARY1_00014374
                                   + piVar18[2]) - local_34[2];
                          puVar19[1] = 0xc4000000;
                          puVar19[8] = 0;
                          *puVar19 = (uVar20 >> 0x13) + 0xf & 0xf | 0x200000 |
                                     ((uVar20 & 0x7ffff) >> 0xf) << 4 | (uVar20 & 0x7ff0) << 4;
                          iVar13 = PVRSRVAllocDeviceMem
                                             (local_188,uVar17,9,0x48,0x10,ppiVar14 + 0x1f);
                          if (iVar13 == 0) {
                            piVar6 = (int *)*ppiVar14[0x1f];
                            PVRSRVMemCopy(piVar6,&DAT_000131e0,0x48);
                            uVar20 = (*(int *)
                                       PTR_ui32uKernelProgramOffsetINITUKERNELSECONDARY_000143a8 +
                                     piVar18[2]) - local_34[2];
                            iVar13 = ppiVar14[5][2];
                            piVar6[1] = 0x1e0001f;
                            *piVar6 = iVar13;
                            piVar6[3] = 0x4000000;
                            piVar6[8] = 0;
                            piVar6[2] = (uVar20 >> 0x13) + 0xf & 0xf | 0x200000 |
                                        ((uVar20 & 0x7ffff) >> 0xf) << 4 | (uVar20 & 0x7ff0) << 4;
                            iVar13 = PVRSRVAllocDeviceMem
                                               (local_188,uVar17,9,0x38,0x10,ppiVar14 + 0x20);
                            if (iVar13 == 0) {
                              puVar19 = (uint *)*ppiVar14[0x20];
                              PVRSRVMemCopy(puVar19,&DAT_00013240,0x38);
                              uVar20 = (*(int *)
                                         PTR_ui32uKernelProgramOffsetINITUKERNELPRIMARY2_000143d0 +
                                       piVar18[2]) - local_34[2];
                              puVar19[1] = 0xc4000000;
                              puVar19[8] = 0;
                              *puVar19 = (uVar20 >> 0x13) + 0xf & 0xf | 0x200000 |
                                         ((uVar20 & 0x7ffff) >> 0xf) << 4 | (uVar20 & 0x7ff0) << 4;
                            }
                          }
                        }
                        iVar13 = PVRSRVAllocDeviceMem(local_188,uVar17,9,0x148,0x10,ppiVar14 + 0x21)
                        ;
                        if (iVar13 == 0) {
                          piVar6 = ppiVar14[0x21];
                          PVRSRVMemCopy(*piVar6,&DAT_00012fa0,0x148);
                          uVar20 = (*(int *)PTR_ui32uKernelProgramOffsetEVENTHANDLER_00014404 +
                                   piVar18[2]) - local_34[2];
                          *(undefined4 *)(*piVar6 + 0x20) = 0;
                          *(undefined4 *)(*piVar6 + 0x24) = 0x100;
                          *(undefined4 *)(*piVar6 + 0x28) = 0x200;
                          *(undefined4 *)*piVar6 = 0xc4000000;
                          *(undefined4 *)(*piVar6 + 4) = 0;
                          *(uint *)(*piVar6 + 0x3c) =
                               (uVar20 >> 0x13) + 0xf & 0xf | 0x200000 |
                               ((uVar20 & 0x7ffff) >> 0xf) << 4 | (uVar20 & 0x7ff0) << 4;
                          uVar20 = local_34[2];
                          uVar21 = (*(int *)PTR_ui32uKernelProgramOffsetTA_TAEVENTHANDLER_000143b8 +
                                   piVar2[2]) - uVar20;
                          uVar22 = piVar3[2] - uVar20;
                          uVar23 = (*(int *)
                                     PTR_ui32uKernelProgramOffset3D_THREEDEVENTHANDLER_000143e8 +
                                   piVar4[2]) - uVar20;
                          uVar20 = piVar5[2] - uVar20;
                          *(uint *)(*piVar6 + 0x34) =
                               (uVar21 >> 0x13) + 0xf & 0xf | 0x200000 |
                               ((uVar21 & 0x7ffff) >> 0xf) << 4 | (uVar21 & 0x7ff0) << 4;
                          *(uint *)(*piVar6 + 0x38) =
                               (uVar22 >> 0x13) + 0xf & 0xf | 0x200000 |
                               ((uVar22 & 0x7ffff) >> 0xf) << 4 | (uVar22 & 0x7ff0) << 4;
                          *(uint *)(*piVar6 + 0x30) =
                               (uVar23 >> 0x13) + 0xf & 0xf | 0x200000 |
                               ((uVar23 & 0x7ffff) >> 0xf) << 4 | (uVar23 & 0x7ff0) << 4;
                          *(uint *)(*piVar6 + 0x2c) =
                               (uVar20 >> 0x13) + 0xf & 0xf | 0x200000 |
                               ((uVar20 & 0x7ffff) >> 0xf) << 4 | (uVar20 & 0x7ff0) << 4;
                          iVar13 = piVar2[2];
                          iVar15 = *(int *)PTR_ui32uKernelProgramOffsetTA_TAHANDLER_00014378;
                          iVar7 = piVar3[2];
                          uVar20 = local_34[2];
                          iVar8 = piVar5[2];
                          iVar9 = piVar4[2];
                          iVar10 = *(int *)PTR_ui32uKernelProgramOffset3D_THREEDHANDLER_000143f4;
                          iVar16 = PVRSRVAllocDeviceMem
                                             (local_188,uVar17,9,0x8c,0x10,ppiVar14 + 0x22);
                          if (iVar16 == 0) {
                            uVar17 = (iVar15 + iVar13) - uVar20;
                            uVar21 = (iVar7 + 0xb0) - uVar20;
                            uVar22 = (iVar10 + iVar9) - uVar20;
                            uVar20 = (iVar8 + 0x680) - uVar20;
                            ppuVar11 = (uint **)ppiVar14[0x22];
                            PVRSRVMemCopy(*ppuVar11,&DAT_00013100,0x8c);
                            (*ppuVar11)[8] = 0;
                            (*ppuVar11)[9] = 0x200;
                            (*ppuVar11)[0xc] = 0x240100;
                            (*ppuVar11)[0xb] = 0x8000f;
                            **ppuVar11 = (uVar22 >> 0x13) + 0xf & 0xf | 0x200000 |
                                         ((uVar22 & 0x7ffff) >> 0xf) << 4 | (uVar22 & 0x7ff0) << 4;
                            (*ppuVar11)[2] =
                                 (uVar17 >> 0x13) + 0xf & 0xf | 0x200000 |
                                 ((uVar17 & 0x7ffff) >> 0xf) << 4 | (uVar17 & 0x7ff0) << 4;
                            (*ppuVar11)[6] =
                                 (uVar21 >> 0x13) + 0xf & 0xf | 0x200000 |
                                 ((uVar21 & 0x7ffff) >> 0xf) << 4 | (uVar21 & 0x7ff0) << 4;
                            puVar19 = *ppuVar11;
                            puVar19[1] = 0xc4000000;
                            puVar19[3] = 0xc4000000;
                            puVar19[5] = 0xc4000000;
                            puVar19[7] = 0xc4000000;
                            (*ppuVar11)[10] = 0;
                            (*ppuVar11)[4] =
                                 (uVar20 >> 0x13) + 0xf & 0xf | 0x200000 |
                                 ((uVar20 & 0x7ffff) >> 0xf) << 4 | (uVar20 & 0x7ff0) << 4;
                            (*ppuVar11)[0xd] = 0x420000;
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            FUN_00011530();
                            uVar17 = ppiVar14[0x1e][2];
                            iVar13 = *ppiVar14[5];
                            iVar15 = PVRSRVAllocDeviceMem
                                               (local_188,local_30[1],0x800002b,0x40000,0x1000,
                                                ppiVar14 + 0x1e6);
                            if ((iVar15 == 0) &&
                               (iVar15 = PVRSRVAllocDeviceMem
                                                   (local_188,local_30[1],0x800002b,0x40000,0x1000,
                                                    ppiVar14 + 0x1e7), iVar15 == 0)) {
                              *(uint *)(iVar13 + 0xd0) = local_30[2];
                              *(uint *)(iVar13 + 0xd4) =
                                   ppiVar14[0x1e6][2] - local_30[2] | 0x80000000;
                              *(uint *)(iVar13 + 0xd8) =
                                   ppiVar14[0x1e7][2] - local_30[2] | 0x80000000;
                            }
                            else {
                              if (ppiVar14[0x1e7] != (int *)0x0) {
                                PVRSRVFreeDeviceMem(local_188,ppiVar14[0x1e7]);
                              }
                              if (ppiVar14[0x1e6] != (int *)0x0) {
                                PVRSRVFreeDeviceMem(local_188,ppiVar14[0x1e6]);
                              }
                            }
                            ppiVar14[0x38] = ppiVar14[0x32];
                            ppiVar14[0x36] = (int *)0x1;
                            ppiVar14[0x37] = (int *)0x0;
                            ppiVar14[0x39] = (int *)0x1;
                            ppiVar14[0x3a] = (int *)0xa58;
                            ppiVar14[0x3b] = (int *)0x0;
                            ppiVar14[0x3c] = (int *)0x1;
                            ppiVar14[0x3d] = (int *)0xacc;
                            ppiVar14[0x3e] = (int *)0x0;
                            ppiVar14[0x3f] = (int *)0x2;
                            ppiVar14[0xf6] = (int *)0x1;
                            ppiVar14[0xf7] = (int *)0x130;
                            ppiVar14[0xf8] = (int *)0x4000;
                            ppiVar14[0xf9] = (int *)0x1;
                            ppiVar14[0xfa] = (int *)0x110;
                            ppiVar14[0xfb] = (int *)0x0;
                            ppiVar14[0xfc] = (int *)0x1;
                            ppiVar14[0xfd] = (int *)0x13c;
                            ppiVar14[0xfe] = (int *)0x0;
                            ppiVar14[0xff] = (int *)0x1;
                            ppiVar14[0x100] = (int *)0xa7c;
                            ppiVar14[0x101] = (int *)0x0;
                            ppiVar14[0x102] = (int *)0x1;
                            ppiVar14[0x103] = (int *)0xa80;
                            ppiVar14[0x104] = (int *)0x0;
                            ppiVar14[0x105] = (int *)0x1;
                            ppiVar14[0x106] = (int *)0xa74;
                            ppiVar14[0x107] = (int *)0x3f8200;
                            ppiVar14[0x108] = (int *)0x1;
                            ppiVar14[0x109] = (int *)0x4e8;
                            ppiVar14[0x10a] = (int *)0x0;
                            ppiVar14[0x10b] = (int *)0x1;
                            ppiVar14[0x10c] = (int *)0xa78;
                            ppiVar14[0x10d] = (int *)0x0;
                            ppiVar14[0x2c] = (int *)0x180;
                            ppiVar14[0x10e] = (int *)0x1;
                            ppiVar14[0x10f] = (int *)0xaac;
                            ppiVar14[0x110] = (int *)0x44;
                            ppiVar14[0x111] = (int *)0x1;
                            ppiVar14[0x112] = (int *)0xabc;
                            ppiVar14[0x113] = (int *)0x4c;
                            ppiVar14[0x2d] = (int *)0x4dd;
                            ppiVar14[0x114] = (int *)0x1;
                            ppiVar14[0x115] = (int *)0x804;
                            ppiVar14[0x116] = (int *)0xffff;
                            ppiVar14[0x117] = (int *)0x1;
                            ppiVar14[0x118] = (int *)0xa00;
                            ppiVar14[0x119] = (int *)0x7c000;
                            uVar20 = local_2c[2];
                            piVar18 = (int *)0xa0c;
                            bVar12 = false;
                            uVar21 = 0xc;
                            while( true ) {
                              uVar22 = uVar21;
                              if (uVar21 < 0x40) {
                                if (!bVar12) {
                                  uVar22 = uVar21 + 1;
                                  ppiVar1 = ppiVar14 + uVar21 * 3 + 0xf6;
                                  if (ppiVar1 != (int **)0x0) {
                                    *ppiVar1 = (int *)0x1;
                                    ppiVar1[1] = piVar18;
                                    ppiVar1[2] = (int *)(uVar20 >> 7 | 0x2000000);
                                  }
                                }
                              }
                              else {
                                bVar12 = true;
                              }
                              piVar18 = piVar18 + 1;
                              if (piVar18 == (int *)0xa34) break;
                              uVar20 = uVar20 + 0x80000;
                              uVar21 = uVar22;
                            }
                            uVar20 = local_28[2];
                            piVar18 = (int *)0xa34;
                            uVar21 = uVar22;
                            if (0x3f < uVar22) goto LAB_00012842;
                            do {
                              uVar22 = uVar21;
                              if (!bVar12) {
                                uVar22 = uVar21 + 1;
                                ppiVar1 = ppiVar14 + uVar21 * 3 + 0xf6;
                                if (ppiVar1 != (int **)0x0) {
                                  *ppiVar1 = (int *)0x1;
                                  ppiVar1[1] = piVar18;
                                  ppiVar1[2] = (int *)(uVar20 >> 7);
                                }
                              }
                              while( true ) {
                                piVar18 = piVar18 + 1;
                                if (piVar18 == (int *)0xa44) {
                                  uVar20 = local_34[2];
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xa48;
                                      ppiVar14[uVar22 * 3 + 0xf8] = (int *)(uVar20 >> 7 | 0x6000000)
                                      ;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  piVar18 = ppiVar14[0x2a];
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xab8;
                                      ppiVar14[uVar22 * 3 + 0xf8] = piVar18;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xb30;
                                      ppiVar14[uVar22 * 3 + 0xf8] = (int *)0x100;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  piVar18 = (int *)ppiVar14[2][2];
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xac4;
                                      ppiVar14[uVar22 * 3 + 0xf8] = piVar18;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xa68;
                                      ppiVar14[uVar22 * 3 + 0xf8] = (int *)(uVar17 & 0xfffffff0);
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xa6c;
                                      ppiVar14[uVar22 * 3 + 0xf8] = (int *)0x3;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xa70;
                                      ppiVar14[uVar22 * 3 + 0xf8] = (int *)0x3d01;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  if (uVar22 < 0x40) {
                                    if (!bVar12) {
                                      ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x1;
                                      ppiVar14[uVar22 * 3 + 0xf7] = (int *)0xa58;
                                      ppiVar14[uVar22 * 3 + 0xf8] = (int *)0x2a04f00f;
                                      uVar22 = uVar22 + 1;
                                    }
                                  }
                                  else {
                                    bVar12 = true;
                                  }
                                  ppiVar14[0x2e] = (int *)0x2;
                                  iVar13 = 1;
                                  if ((uVar22 < 0x40) && (!bVar12)) {
                                    ppiVar14[0x1b6] = (int *)0x1;
                                    ppiVar14[0x1b7] = (int *)0xa58;
                                    ppiVar14[0x1b8] = (int *)0x0;
                                    ppiVar14[0x1b9] = (int *)0x1;
                                    ppiVar14[0x1ba] = (int *)0xa68;
                                    ppiVar14[0x1bb] = (int *)0x0;
                                    ppiVar14[0x1bc] = (int *)0x1;
                                    ppiVar14[0x1bd] = (int *)0xa6c;
                                    ppiVar14[0x1be] = (int *)0x0;
                                    ppiVar14[uVar22 * 3 + 0xf6] = (int *)0x2;
                                    ppiVar14[0x1bf] = (int *)0x1;
                                    ppiVar14[0x1c0] = (int *)0xa70;
                                    ppiVar14[0x1c1] = (int *)0x0;
                                    ppiVar14[0x1c2] = (int *)0x1;
                                    ppiVar14[0x1c3] = (int *)0xa48;
                                    ppiVar14[0x1c4] = (int *)0x0;
                                    ppiVar14[0x1c5] = (int *)0x1;
                                    ppiVar14[0x1c6] = (int *)0x130;
                                    ppiVar14[0x1c7] = (int *)0x0;
                                    ppiVar14[0x1c8] = (int *)0x1;
                                    ppiVar14[0x1c9] = (int *)0x110;
                                    ppiVar14[0x1ca] = (int *)0x0;
                                    ppiVar14[0x1cb] = (int *)0x1;
                                    ppiVar14[0x1cc] = (int *)0x0;
                                    ppiVar14[0x1cd] = (int *)0x0;
                                    ppiVar14[0x1ce] = (int *)0x2;
                                    iVar13 = SGXDevInitPart2(local_188,ppiVar14);
                                  }
                                  goto LAB_000118f7;
                                }
                                uVar20 = uVar20 + 0x80000;
                                uVar21 = uVar22;
                                if (uVar22 < 0x40) break;
LAB_00012842:
                                bVar12 = true;
                              }
                            } while( true );
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    iVar13 = 2;
LAB_000118f7:
    PVRSRVFreeUserModeMem(ppiVar14);
    return iVar13;
  }
  goto LAB_00011780;
}


INFO  ANALYZING changes made by post scripts: /tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad/emgd_rpm/usr/lib/libemgdsrv_init.so.1.5.15.3226 (HeadlessAnalyzer)  
INFO  REPORT: Post-analysis succeeded for file: /tmp/claude-1000/-Projects-Poulsbo/476a621f-66ff-423a-96f0-a1dca3ae4adf/scratchpad/emgd_rpm/usr/lib/libemgdsrv_init.so.1.5.15.3226 (HeadlessAnalyzer)  
INFO  REPORT: Save succeeded for: /libemgdsrv_init.so.1.5.15.3226 (sgx:/libemgdsrv_init.so.1.5.15.3226) (HeadlessAnalyzer)  
INFO  REPORT: Import succeeded (HeadlessAnalyzer)  
