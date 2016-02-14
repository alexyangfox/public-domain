public U0 LtfProfile(U8 *filename,U64 flags)
{
  U64 i,*cmd_stats=CAlloc(sizeof(U64)*LTFT_NUM_TYPES),
       *flags_stats=CAlloc(sizeof(U64)*64),
       *type_flags_stats=CAlloc(sizeof(U64)*64);
//Note, word wrap is determined by
//l->win_task when a $LK,"Ltf","MN:Ltf"$ is recalculated
//use $LK,"LtfRecalc","MN:LtfRecalc"$().
  Ltf *l=LtfRead(filename,flags);

//l->root which is equal to l is the
//header of the Queue and represents the end-of-file marker.
  LtfEntry *ll=l->root.next;
  while (ll!=l) {
    cmd_stats[ll->type_u8]++;
    for (i=0;i<64;i++)
      if (Bt(&ll->flags,i))
	flags_stats[i]++;
    for (i=16;i<32;i++)
      if (Bt(&ll->type,i))
	type_flags_stats[i]++;
    ll=ll->next;
  }
  LtfDel(l);

  "$$FG,PURPLE$$-------%s-------\n",filename;
  "$$FG,GREEN$$The lowest byte of the 32-bit 'll->type', '$$FG,PURPLE$$ll->type.u8[0]$$FG,GREEN$$', is cmd and accessed with the union '$$FG,PURPLE$$ll->type_u8$$FG,GREEN$$'."
  "See $$LK,\"LtfEntry\",\"MN:LtfEntry\"$$, "
  "$$LK,\"Ltf Type Defines\",\"MN:LTFT_TEXT\"$$ and $$LK,\"Ltf Type Codes\",\"FF:::/LT/Adam/Ltf/Ltf.APZ,ST_LTF_CMDS\"$$.\n$$FG$$";
  for (i=0;i<LTFT_NUM_TYPES;i++)
    if (cmd_stats[i])
      "%4Z:%d\n",i,"ST_LTF_CMDS",cmd_stats[i];

  "\n$$FG,GREEN$$'$$FG,PURPLE$$ll->type.u8[1]$$FG,GREEN$$' is the screen color attr of the entry."
  "'$$FG,PURPLE$$ll->type.u16[1]$$FG,GREEN$$' is some flags for blinking and stuff."
  "See $$LK,\"Ltf Type Flag Defines\",\"MN:LTFLT_BLINK\"$$.\n$$FG$$";

  for (i=16;i<32;i++)
    if (type_flags_stats[i])
      "%4d:%d\n",i,type_flags_stats[i];

  "$$FG,GREEN$$\n'$$FG,PURPLE$$ll->flags$$FG,GREEN$$' is 64-bit."
  "See $$LK,\"Ltf Flag Defines\",\"MN:LTFLf_AUX_STR\"$$ and $$LK,\"Ltf Flag Codes\",\"FF:::/LT/Adam/Ltf/Ltf.APZ,ST_LTF_FLAGS\"$$.\n$$FG$$";
  for (i=0;i<64;i++)
    if (flags_stats[i])
      "%4Z:%d\n",i,"ST_LTF_FLAGS",flags_stats[i];

  Free(cmd_stats);
  Free(flags_stats);
  PressAKey;
  NewLine;
}


//See $LK,"TipOfDay","MN:TipOfDay"$.

//Note: Not all $LK,"LtfEntry","MN:LtfEntry"$'s are full-sized nodes.
//Some are $LK,"MAlloc","MN:MAlloc"$ed with a smaller size to save
//mem.	They all have at least the size $LK,"LtfEntryBase","MN:LtfEntryBase"$.

//Note: $LK,"LtfEntry","MN:LtfEntry"$'s should be allocated from the
//heap of the owning task, l->mem_task.

//The flag arrays $LK,"ltf_tag_types","MN:ltf_tag_types"$, $LK,"ltf_form_types","MN:ltf_form_types"$, etc
//are useful.

LtfProfile("::/LT/Doc/ChangeLog.TXZ",0);
LtfProfile("::/LT/Doc/ChangeLog.TXZ",LTFF_PLAIN_TEXT);
