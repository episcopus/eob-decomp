/* Unit at overlay 1 of EOB.EXE (segment 2102, 9864 bytes, 16 functions): the party display and
   inventory overlay (src/EOV1.ASM's header says what it does). The unit is C, compiled by TCC
   1.00 (escalation eov1, 2026-11-02): every function has TCC's frame, and ovl1:1153 reads a far
   array with the 16-bit displacement only TCC's own assembler writes (26 8B 87 05 00 at
   ovl1:1277), which TASM cannot. src/EOV1.ASM was written first, as assembly transcribed from
   the disassembly; it stays until this file has each function in C (playbook 30). Every
   function here is a stub until it is written in C. */

/*@flags -ml -O -Z -v */

/*@ ovl1:0000 stub */ void RotateOutMember(void) {}
/*@ ovl1:00DD stub */ void BuildSpellSlots(void) {}
/*@ ovl1:025D stub */ void SpendSpellSlot(void) {}
/*@ ovl1:0416 stub */ void MemberCommand(void) {}
/*@ ovl1:0881 stub */ void ShowItemName(void) {}
/*@ ovl1:097A stub */ void SpellSlotScreen(void) {}
/*@ ovl1:10DE stub */ void BoolString(void) {}
/*@ ovl1:1107 stub */ void SaveOptionField(void) {}
/*@ ovl1:112D stub */ void DrawOptionField(void) {}
/*@ ovl1:1153 stub */ void Ovl1_1153(void) {}
/*@ ovl1:13F7 stub */ void ShowMessage(void) {}
/*@ ovl1:1516 stub */ void FreeFarList(void) {}
/*@ ovl1:1551 stub */ void DrawListNode(void) {}
/*@ ovl1:1691 stub */ void Ovl1_1691(void) {}
/*@ ovl1:1ACD stub */ void Ovl1_1ACD(void) {}
/*@ ovl1:243A stub */ void ChooseListEntry(void) {}
