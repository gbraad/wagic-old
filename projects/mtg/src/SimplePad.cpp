#include "../include/config.h"
#include "../include/SimplePad.h"
#include "JTypes.h"
#include "../include/GameApp.h"
#include "../include/Translate.h"


#define ALPHA_COLUMNS 8
#define ALPHA_ROWS 8

#define KPD_UP 0
#define KPD_DOWN 1
#define KPD_LEFT 2
#define KPD_RIGHT 3

SimpleKey::SimpleKey( string _ds, int _id){
  displayValue = _ds; id = _id; 
  for(int x=0;x<4;x++) 
    adjacency[x] = KPD_NOWHERE;
}

void SimplePad::linkKeys(int from, int to, int dir){

  if(keys[from] && keys[to]){
  keys[from]->adjacency[dir] = to;
  switch(dir){
    case KPD_UP:
    case KPD_LEFT:
      dir++;
      break;
    default:
      dir--;
  }
  keys[to]->adjacency[dir] = from;
  }
}

SimplePad::SimplePad(){
  nbitems = 0;
  bActive = false;
  selected = 0;
  priorKey = 0;
  cursor = 0;
  bShowCancel = false;
  bShowNumpad = false;
  bCapslock = true;
  char buf[2];
  buf[1] = '\0';
  SimpleKey * k;

  for(int x=0;x<KPD_MAX;x++)
    keys[x] = NULL;

  //Add the alphabet. We cheat a bit here.
  for(int x='a';x<='z';x++)
  {
    buf[0] = x;
    k=Add(buf,x);
    int idx = x-'a';

    if(idx > KPD_A)
      k->adjacency[KPD_LEFT] = idx-1;
    if(idx < KPD_Z)
      k->adjacency[KPD_RIGHT] = idx+1;
    if(idx > ALPHA_COLUMNS)
      k->adjacency[KPD_UP] = idx-1-ALPHA_COLUMNS;
    else 
      k->adjacency[KPD_UP] = KPD_INPUT;
    if(idx < 25-ALPHA_COLUMNS)
      k->adjacency[KPD_DOWN] = idx+1+ALPHA_COLUMNS;
  }

  Add("Spacebar",KPD_SPACE);
  
  for(int x=25-ALPHA_COLUMNS;x<26;x++)
    keys[x]->adjacency[KPD_DOWN] = KPD_SPACE;

  k=Add("Confirm",KPD_OK);
  keys[KPD_Z]->adjacency[KPD_RIGHT] = KPD_OK;
  k->adjacency[KPD_UP] = KPD_CAPS;
  k->adjacency[KPD_LEFT] = KPD_Z;
  k->adjacency[KPD_DOWN] = KPD_CANCEL;

  k=Add("Cancel",KPD_CANCEL);
  k->adjacency[KPD_UP] = KPD_OK;
  k->adjacency[KPD_LEFT] = KPD_SPACE;

  k=Add("Del",KPD_DEL);
  keys[KPD_I]->adjacency[KPD_RIGHT] = KPD_DEL;   
  k->adjacency[KPD_UP] = KPD_9;
  k->adjacency[KPD_DOWN] = KPD_CAPS;
  k->adjacency[KPD_LEFT] = KPD_I;
  
  k=Add("Caps",KPD_CAPS);
  keys[KPD_R]->adjacency[KPD_RIGHT] = KPD_CAPS;
  keys[KPD_R]->adjacency[KPD_DOWN] = KPD_Z; 
  k->adjacency[KPD_UP] = KPD_DEL;
  k->adjacency[KPD_DOWN] = KPD_OK;
  k->adjacency[KPD_LEFT] = KPD_R;


  for(int x=0;x<10;x++){
    buf[0] = '0'+x;
    Add(buf,KPD_0+x);
    if(x < 8)
      linkKeys(KPD_0+x,KPD_A+x,KPD_DOWN);
    if(x > 0)
      linkKeys(KPD_0+x,KPD_0+x-1,KPD_LEFT);
  }

  keys[KPD_8]->adjacency[KPD_DOWN] = KPD_DEL;
  keys[KPD_9]->adjacency[KPD_DOWN] = KPD_DEL;

  keys[KPD_0]->adjacency[KPD_LEFT] = KPD_NOWHERE;   
  keys[KPD_A]->adjacency[KPD_LEFT] = KPD_NOWHERE;   
  keys[KPD_J]->adjacency[KPD_LEFT] = KPD_NOWHERE;   
  keys[KPD_S]->adjacency[KPD_LEFT] = KPD_NOWHERE;   

}

SimplePad::~SimplePad()
{
  for(int x=0;x<KPD_MAX;x++)
    SAFE_DELETE(keys[x]);
}


SimpleKey * SimplePad::Add(string display, unsigned char id){
  if(nbitems >= KPD_MAX)
    return NULL;

  keys[nbitems++] = NEW SimpleKey(display,id);
  return keys[nbitems-1];
}
void SimplePad::pressKey(unsigned char key){
  string input = "";

  if(isalpha(key))  {
    if(bCapslock)
      input += toupper(key);
    else
      input += key;
 
   if(cursor < buffer.size())
      cursor++;

    buffer.insert(cursor,input);
  }
  else if(key == KPD_CAPS)
    bCapslock = !bCapslock;
  else if(key == KPD_DEL) {
    if(!buffer.size())
      return;

    if(cursor >= buffer.size()) {
      cursor = buffer.size();
      buffer = buffer.substr(0,cursor-1);
    }
    else
      buffer = buffer.substr(0,cursor) + buffer.substr(cursor+1);
    
    cursor--;
  }
  else if(key == KPD_OK) 
    Finish();
  else if(key == KPD_CANCEL) {
    bCanceled = true;
    Finish();
  }

}
void SimplePad::MoveSelection(unsigned char moveto)
{
  if(!bShowNumpad && moveto >= KPD_0 && moveto <= KPD_9 )
    moveto = KPD_INPUT;
  else if(!bShowCancel && moveto == KPD_CANCEL )
    moveto = KPD_SPACE;
  
  if(selected < KPD_MAX && selected >= 0) 
    priorKey = selected;

  if(moveto < KPD_MAX) {
    selected = moveto;
  }
  else if(moveto == KPD_INPUT)
    selected = KPD_INPUT;
}

void SimplePad::Update(float dt){
  JGE * mEngine = JGE::GetInstance();
  
  //Start button changes capslock setting.
  if(mEngine->GetButtonClick(PSP_CTRL_START))
  {
    if(selected != KPD_OK)
      selected = KPD_OK;
    else
      Finish();
   }
  else if(mEngine->GetButtonClick(PSP_CTRL_SELECT)){
    bCapslock = !bCapslock;
  }

  if(selected == KPD_SPACE){
    if(bShowCancel && mEngine->GetButtonClick(PSP_CTRL_RIGHT))
      selected = KPD_CANCEL;
    else if (mEngine->GetButtonClick(PSP_CTRL_LEFT)||mEngine->GetButtonClick(PSP_CTRL_RIGHT)
      ||mEngine->GetButtonClick(PSP_CTRL_UP)||mEngine->GetButtonClick(PSP_CTRL_DOWN))
      selected = priorKey;
  } //Moving within/from the text field.
  else if(selected == KPD_INPUT){
    if (mEngine->GetButtonClick(PSP_CTRL_DOWN) )
      selected = priorKey;
    if (mEngine->GetButtonClick(PSP_CTRL_LEFT)){
      if(cursor > 0)
        cursor--;
    }
    else if (mEngine->GetButtonClick(PSP_CTRL_RIGHT)){
      if(cursor < buffer.size())
        cursor++;
    }
  }
  else if(selected >= 0 && keys[selected]){
    if (mEngine->GetButtonClick(PSP_CTRL_LEFT))
      MoveSelection(keys[selected]->adjacency[KPD_LEFT]);
    else if (mEngine->GetButtonClick(PSP_CTRL_RIGHT))
      MoveSelection(keys[selected]->adjacency[KPD_RIGHT]);
    if (mEngine->GetButtonClick(PSP_CTRL_DOWN))
      MoveSelection(keys[selected]->adjacency[KPD_DOWN]);
    else if (mEngine->GetButtonClick(PSP_CTRL_UP))
      MoveSelection(keys[selected]->adjacency[KPD_UP]);
  }
 

  //These bits require a valid key...
  if(selected >= 0 && selected < nbitems && keys[selected]){
  if (mEngine->GetButtonClick(PSP_CTRL_CIRCLE))
    pressKey(keys[selected]->id);
  }
  if (buffer.size() > 0 && mEngine->GetButtonClick(PSP_CTRL_CROSS))
    buffer = buffer.substr(0,buffer.size() - 1);
  if (buffer.size() && mEngine->GetButtonClick(PSP_CTRL_LTRIGGER)){
      if(cursor > 0)
        cursor--;
  }
  else if (mEngine->GetButtonClick(PSP_CTRL_RTRIGGER)){
    if(cursor < buffer.size())
        cursor++;
    else{
      buffer += ' ';
      cursor = buffer.size();
    }
  }

  mX = 50;
  mY = 50;

  //Clear input buffer.
  mEngine->ResetInput();
}
void SimplePad::Start(string value, string * _dest) {
  bActive = true;
  bCanceled=false;
  buffer = value;
  original = buffer;
  dest = _dest;
  cursor = buffer.size();
  //Clear input buffer.
  JGE * mEngine = JGE::GetInstance();
  mEngine->ResetInput();
}

string SimplePad::Finish() {
  bActive = false;
  
  //Clear input buffer.
  JGE * mEngine = JGE::GetInstance();
  mEngine->ResetInput();

  //Return result.
  if(bCanceled){
    dest = NULL;
    return original;
  }
  if(dest != NULL){
    dest->clear(); dest->insert(0,buffer);
    dest = NULL;
  }
  return buffer;
}

void SimplePad::Render(){
  //This could use some cleaning up to make margins more explicit
  JLBFont * mFont = resources.GetJLBFont("f3");

  int offX = 0, offY = 0;   
  int kH = mFont->GetHeight();
  int hSpacing = mFont->GetStringWidth("W");
  int rowLen = mFont->GetStringWidth("JKLMNOPQR") + 14*7; 
  int vSpacing = 0;
  int kW = hSpacing;
  
  JRenderer * renderer = JRenderer::GetInstance();

  
  vSpacing = kH+8;
 

  offY = vSpacing;
  if(bShowNumpad)
    offY += kH+14;
  //Draw Keypad Background.
  renderer->FillRoundRect(mX-kW,mY-kH,(kW+12)*11,(kH+14)*5+offY,2,options[Metrics::POPUP_MENU_FC].asColor(ARGB(180,0,0,0)));
  offY = vSpacing;
  //Draw text entry bubble
  renderer->FillRoundRect(mX-kW/2,mY+offY,(kW+12)*9+kW/2,kH,2,options[Metrics::KEYPAD_FCH].asColor(ARGB(255,255,255,255)));

  //Draw text-entry title, if we've got one.
  if(title != ""){
    mFont->DrawString(title.c_str(),mX,mY);
  }
    mY+=kH+12;

  //Draw cursor.
  if(cursor < buffer.size())
  {
    kW = mFont->GetStringWidth(buffer.substr(cursor,1).c_str());
    renderer->FillRoundRect(mX+mFont->GetStringWidth(buffer.substr(0,cursor).c_str()),mY+kH-4,
      kW,4,2,options[Metrics::KEY_FCH].asColor(ARGB(150,150,150,0)));    
  }
  else
  {
    cursor = buffer.size();
    renderer->FillRoundRect(mX+mFont->GetStringWidth(buffer.substr(0,cursor).c_str()),mY+kH-4,
      kW,4,2,options[Metrics::KEY_FCH].asColor(ARGB(150,150,150,0)));    
  }
  
  mFont->SetColor(options[Metrics::KEYPAD_TC].asColor(ARGB(255,0,0,0)));
  mFont->DrawString(buffer.c_str(),mX,mY);
  offY += kH + 12;

  if(!bShowNumpad)
    vSpacing -= kH + 12;
    
  for(int x=0;x<nbitems;x++)
    if(keys[x]){   

      if((x == KPD_CANCEL && !bShowCancel) || (x >= KPD_0 && x <= KPD_9 && !bShowNumpad))
        continue;
      
      switch(x){
        case KPD_0:  
          offX = 0;
          offY = vSpacing;
          break;
        case KPD_A:  
          offX = 0;
          offY = vSpacing+(kH+12)*1;
          break;
        case KPD_J:
          offX = 0;
          offY = vSpacing+(kH+12)*2;
          break;
        case KPD_S:
          offX = 0;
          offY = vSpacing+(kH+12)*3;
          break;
        case KPD_SPACE:  
          offX = 0;
          offY = vSpacing+(kH+12)*4;
          break;
        case KPD_OK:
          offX = rowLen + hSpacing;
          offY = vSpacing+(kH+12)*3;
          break;
        case KPD_CANCEL:
          offX = rowLen + hSpacing;
          offY = vSpacing+(kH+12)*4;
          break;
        case KPD_DEL:
          offX = rowLen + hSpacing;
          offY = vSpacing+(kH+12)*1;
          break;
        case KPD_CAPS:
          offX = rowLen + hSpacing;
          offY = vSpacing+(kH+12)*2;
          break;
      }

      kW = mFont->GetStringWidth(keys[x]->displayValue.c_str());
      //Render a key.
      if(x != selected){
        renderer->FillRoundRect(mX+offX-4,mY+offY-4,kW+8,kH+4,2,options[Metrics::POPUP_MENU_FC].asColor(ARGB(180,50,50,50)));
        mFont->SetColor(options[Metrics::POPUP_MENU_TCH].asColor(ARGB(255,255,255,0)));
      }
      else{
        renderer->FillRoundRect(mX+offX-4,mY+offY-4,kW+8,kH+4,2,options[Metrics::POPUP_MENU_FC].asColor(ARGB(255,100,100,100)));
        mFont->SetColor(options[Metrics::POPUP_MENU_TC].asColor(ARGB(255,255,255,255)));
      }

      char vkey[2];
      vkey[1] = '\0';
      vkey[0] = keys[x]->id;

      
      if(isalpha(vkey[0])) {
        if(bCapslock) vkey[0] = toupper(vkey[0]);
        mFont->DrawString(vkey,mX+offX,mY+offY);
      }
      else
        mFont->DrawString(keys[x]->displayValue.c_str(),mX+offX,mY+offY);
      offX += kW + 14;
    }
}

unsigned int SimplePad::cursorPos(){
  if(cursor > buffer.size())
    return buffer.size();

  return cursor;
}
