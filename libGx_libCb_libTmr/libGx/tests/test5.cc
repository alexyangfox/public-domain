#include <assert.h>
#include <iostream>

#include <libGx/GxMainInterface.hh>
#include <libGx/GxDisplay.hh>
#include <libGx/GxMainWin.hh>
#include <libGx/GxButton.hh>
#include <libGx/GxPulldownMenu.hh>
#include <libGx/GxRadioBox.hh>

GxMainInterface *pMainInt = NULL;

void QuitFun(void)
{
  pMainInt->EndEventLoop();
}

void ItemCallback(int item)
{
  std::cout << "got item: " << item << std::endl;
}

int main(int argc, char **argv)
{
  GxMainInterface mainInt("Test5");
  pMainInt = &mainInt;

  assert( mainInt.Initialize(argc, argv) );
  assert( mainInt.OpenAllocateAll() );

  GxMainWin *pMain = new GxMainWin(mainInt.dispVector[0]);
  pMain->Resize(350,350);
  pMain->SetGeomControl( GxBasic(GX_WD_FIXED, GX_HT_FIXED, GX_H_FIXED,
				 GX_V_FIXED) );

  GxPulldownMenu *pMenu = new GxPulldownMenu(pMain);
  pMenu->SetNoneSelectedText("Select Me");
  pMenu->Move(75,50);
  pMenu->SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED,
				   GX_FLOW_UP, 0,0, 1,2));

  GxPulldownItem *pItem;
  pItem = new GxPulldownItem;
  pItem->SetLabel("Item One");
  pItem->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 1) );
  pMenu->AddItem(pItem);
  
  pItem = new GxPulldownItem;
  pItem->SetLabel("Item Two");
  pItem->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 2) );
  pMenu->AddItem(pItem);

  pItem = new GxPulldownItem;
  pItem->SetLabel("Item Three");
  pItem->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 3) );
  pMenu->AddItem(pItem);

  pItem = new GxPulldownItem;
  pItem->SetLabel("Item Four");
  pItem->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 4) );
  pMenu->AddItem(pItem);

  pItem = new GxPulldownItem;
  pItem->SetLabel("Item Five");
  pItem->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 5) );
  pMenu->AddItem(pItem);

  pItem = new GxPulldownItem;
  pItem->SetLabel("Item Six");
  pItem->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 6) );
  pMenu->AddItem(pItem);

  GxRadioBox *pBox = new GxRadioBox(pMain);
  pBox->SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED,
				  GX_FLOW_UP, 1,1,1,1) );
  pBox->SetNum(3);
  pBox->SetFormating(true); //in rows

  GxRadioButton *pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item One");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 1) );
  
  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Two");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 2) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Three");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 3) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Four");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 4) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Five");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 5) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Six");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 6) );

  pBox = new GxRadioBox(pMain);
  pBox->SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT, GX_H_CENTERED,
				  GX_FLOW_UP, 1,1,1,1) );
  pBox->SetNum(3); //per column
  pBox->SetFormating(false); //in columns

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item One");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 1) );
  
  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Two");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 2) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Three");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 3) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Four");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 4) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Five");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 5) );

  pTButton = new GxRadioButton(pBox);
  pTButton->SetLabel("Item Six");
  pTButton->cb.Assign( CbVoidPlainObj<int>(ItemCallback, 6) );

  GxButton *pButton = new GxButton(pMain);
  pButton->SetLabel("Quit");
  pButton->cb.Assign( CbVoidPlain(QuitFun) );
  pButton->SetGeomControl( GxSIBasic(GX_WD_INT, GX_HT_INT,
				     GX_H_CENTERED, GX_FLOW_DOWN,
				     0,0,1,1) );

  pMain->Place();
  pMain->Create();
  pMain->Display();

  mainInt.EventLoop();

  //we should delete everything

  return 0;
};

