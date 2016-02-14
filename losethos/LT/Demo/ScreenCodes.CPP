//See $LK,"ExtScreenCodes","FA:::/LT/Adam/Gr/GrScreen.APZ,ExtScreenCodes"$ for the format
//of the U32 you pass to PlotWinU8()

//See $LK,"::/LT/Demo/ExtendedChars.CPZ","FI:::/LT/Demo/ExtendedChars.CPZ"$
//and $LK,"::/LT/Demo/Graphics/FontEd.CPZ","FI:::/LT/Demo/Graphics/FontEd.CPZ"$.
//Hold ALT and press a 3 digit decimal number
//or press <CTRL-ALT-A>.

U0 DrawIt(TaskStruct *task,GrDC *dc)
{
  nounusedwarn dc;
  I64 i;
  for (i=0;i<256;i++)
    PlotWinU8(task,i+BLACK<<12+WHITE<<8,(i&15)*2,(i/16)*2);
}

U0 ScreenCodes()
{
  SettingsPush; //See $LK,"SettingsPush","MN:SettingsPush"$
  LtfClear;
  Fs->draw_it=&DrawIt;
  GetChar(NULL,FALSE);
  SettingsPop;
}

ScreenCodes;
