#include "../include/config.h"
#include "../include/utils.h"
#include "../include/MTGDeck.h"
#include "../include/GameOptions.h"
#include "../include/OptionItem.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <JGE.h>

const char * Options::optionNames[] = {
//Global options
  "Profile",
  "prx_handler",
  "prx_rimom",
  "prx_eviltwin",
  "prx_rnddeck",
//Options set on a per-profile basis
  "Theme",
  "Mode",
  "musicVolume",
  "sfxVolume",
  "difficulty",
  "displayOSD",
  "closed_hand",
  "hand_direction",
  "mana_display",
  "reverse_triggers",
  "disable_cards",
  "interruptSeconds",
  "interruptMySpells",
  "interruptMyAbilities",
//General interrupts
  "interruptBeforeBegin",
  "interruptUntap",
  "interruptUpkeep",
  "interruptDraw",
  "interruptFirstMain",
  "interruptBeginCombat",
  "interruptAttackers",
  "interruptBlockers",
  "interruptDamage",
  "interruptEndCombat",
  "interruptSecondMain",
  "interruptEndTurn",
  "interruptCleanup",
  "interruptAfterEnd",
};
int Options::getID(string name){
  if(!name.size())
    INVALID_OPTION;

  std::transform(name.begin(),name.end(),name.begin(),::tolower);

  //Is it a named option?
  for(int x = 0; x < LAST_NAMED; x++){
    string lower = Options::optionNames[x];
    std::transform(lower.begin(),lower.end(),lower.begin(),::tolower);

    if(lower == name)
      return x;
  }

  //Is it an unlocked set?
  string setname = name.substr(strlen("unlocked_"));
  if(MtgSets::SetsList && MtgSets::SetsList->nb_items){
    int unlocked = MtgSets::SetsList->find(setname);
    if(unlocked != -1)
      return Options::optionSet(unlocked);  
  }

  //Failure.
  return INVALID_OPTION;
}

string Options::getName(int option){
  //Invalid options
  if(option < 0)
    return "";

  //Standard named options
  if(option < LAST_NAMED)
    return optionNames[option];
  
  //Unlocked sets.
  if(MtgSets::SetsList){
    int setID = option - SET_UNLOCKS;
    if(setID >= 0 && setID < MtgSets::SetsList->nb_items){
      char buf[512];
      sprintf(buf,"unlocked_%s",MtgSets::SetsList->values[setID].c_str());
      return buf;
    }
  }

  //Failed.
  return "";
}

int Options::optionSet(int setID){
  //Sanity check if possible
  if(setID < 0 || (MtgSets::SetsList && setID > MtgSets::SetsList->nb_items))
    return INVALID_OPTION;

  return SET_UNLOCKS + setID;  
}

int Options::optionInterrupt(int gamePhase){
  //Huge, nearly illegible switch block spread out to improve readability.
  switch(gamePhase){
      case Constants::MTG_PHASE_BEFORE_BEGIN:
        return INTERRUPT_BEFOREBEGIN;

      case Constants::MTG_PHASE_UNTAP:
        return INTERRUPT_UNTAP;

      case Constants::MTG_PHASE_UPKEEP:
        return INTERRUPT_UPKEEP;

      case Constants::MTG_PHASE_DRAW:
        return INTERRUPT_DRAW;

      case Constants::MTG_PHASE_FIRSTMAIN:
        return INTERRUPT_FIRSTMAIN;

      case Constants::MTG_PHASE_COMBATBEGIN:
        return INTERRUPT_BEGINCOMBAT;

      case Constants::MTG_PHASE_COMBATATTACKERS:
        return INTERRUPT_ATTACKERS;

      case Constants::MTG_PHASE_COMBATBLOCKERS:
        return INTERRUPT_BLOCKERS;

      case Constants::MTG_PHASE_COMBATDAMAGE:
        return INTERRUPT_DAMAGE;

      case Constants::MTG_PHASE_COMBATEND:
        return INTERRUPT_ENDCOMBAT;

      case Constants::MTG_PHASE_SECONDMAIN:
        return INTERRUPT_SECONDMAIN;

      case Constants::MTG_PHASE_ENDOFTURN:
        return INTERRUPT_ENDTURN;

      case Constants::MTG_PHASE_CLEANUP:
        return INTERRUPT_CLEANUP;

      case Constants::MTG_PHASE_AFTER_EOT:
        return INTERRUPT_AFTEREND;
  }

  return INVALID_OPTION;
}

GameOption::GameOption(int value) : number(value){}
GameOption::GameOption(string value) : number(0), str(value) {}
GameOption::GameOption(int num, string str) : number(num), str(str) {}

bool GameOption::isDefault(){
  string test = str;
  std::transform(test.begin(),test.end(),test.begin(),::tolower);
  
  if(!test.size() || test == "default")
    return true;

  return false;
}

PIXEL_TYPE GameOption::asColor(PIXEL_TYPE fallback)
{
  unsigned char color[4];
  string temp;
  int subpixel=0;

  //The absolute shortest a color could be is 5 characters: "0,0,0" (implicit 255 alpha)
  if(str.length() < 5)
    return fallback;

  for(size_t i=0;i<str.length();i++)  {
    if(isspace(str[i]))
      continue;
    if(str[i] == ','){
      if(temp == "")
        return fallback;
      color[subpixel] = (unsigned char) atoi(temp.c_str());
      temp = "";
      subpixel++;
      continue;
    }
    else if(!isdigit(str[i]))
      return fallback;
    if(subpixel > 3)
      return fallback;
    temp += str[i];
  }

  if(temp != "")
    color[subpixel] = (unsigned char) atoi(temp.c_str());
  if(subpixel == 2)
    color[3] = 255;

  return ARGB(color[3],color[0],color[1],color[2]);  
}

bool GameOption::read(string input){
  bool bNumeric = true;
  
  if(!input.size()){
    return true; //Default reader doesn't care about invalid formatting.
  }

  //Is it a number?
  for(size_t x=0;x<input.size();x++) {
    if(!isdigit(input[x])) {
      bNumeric = false;
      break;
    }
  }

  if(bNumeric)
    number = atoi(input.c_str());
  else
    str = input;
  return true;
}
string GameOption::menuStr(){
  if(number){
    char buf[12];
    sprintf(buf,"%d",number);
  }

  if(str.size())
    return str;

  return "0";
}
bool GameOption::write(std::ofstream * file, string name){
  char writer[1024];

  if(!file)
    return false;

   if(str ==""){
     	if(number == 0) //This is absolutely default. No need to write it.
	      return true;

      //It's a number!
      sprintf(writer,"%s=%d\n", name.c_str(), number);
	  }
	  else
	    sprintf(writer,"%s=%s\n", name.c_str(), str.c_str());
  
  (*file)<<writer;
  return true;
}


GameOptions::GameOptions(string filename){
  mFilename = filename;
  values.reserve(Options::LAST_NAMED); //Reserve space for all named options.
  load();
}

int GameOptions::load(){
  std::ifstream file(mFilename.c_str());
  std::string s;
  
  if(file){
    while(std::getline(file,s)){
      if (!s.size()) continue;
      if (s[s.size()-1] == '\r') s.erase(s.size()-1); //Handle DOS files
      int found =s.find("=");
      string name = s.substr(0,found);
      string val = s.substr(found+1);
      int id = Options::getID(name);      
      if(id == INVALID_OPTION)
        continue;

      (*this)[id].read(val);
    }
    file.close();
  }
  return 1;
}
int GameOptions::save(){
  std::ofstream file(mFilename.c_str());
  if (file){
    for ( int x=0; x < (int) values.size(); x++ ){
      
      //Check that this is a valid option.
      string name = Options::getName(x);
      GameOption * opt = get(x);
      if(!name.size() || !opt)
        continue;

      //Save it.
      opt->write(&file, name);
    }
    
    file.close();
  }
  return 1;
}

GameOption& GameOptions::operator[](int optionID){
  GameOption * go = get(optionID);
  if(!go)
    return GameSettings::invalid_option;

  return *go;
}

GameOption * GameOptions::get(int optionID) {
  GameOption * go = NULL;
  GameOptionEnum * goEnum = NULL;

  //Invalid options!
  if(optionID < 0)
    return NULL;

  //Option doesn't exist, so build it
  int x = (int) values.size();
  values.reserve(optionID);

  while(x <= optionID){
      switch(optionID){
        case Options::HANDDIRECTION:
          goEnum = NEW GameOptionEnum();
          goEnum->def = OptionHandDirection::getInstance();
          go = goEnum;
          break;
        case Options::CLOSEDHAND:
          goEnum = NEW GameOptionEnum();
          goEnum->def = OptionClosedHand::getInstance();
          go = goEnum;
          break;
        case Options::MANADISPLAY:
          goEnum = NEW GameOptionEnum();
          goEnum->def = OptionManaDisplay::getInstance();
          go = goEnum;
          break;
       default:
          go = NEW GameOption();
          break;
    }
    values.push_back(go);
    x++;
  }

  return values[optionID];
}

GameOptions::~GameOptions(){
  for(vector<GameOption*>::iterator it=values.begin();it!=values.end();it++)
    SAFE_DELETE(*it);
  values.clear();
}

GameSettings options;

GameSettings::GameSettings()
{
  globalOptions = NULL;
  theGame = NULL;
  profileOptions = NULL;
  //reloadProfile should be before using options.
}

GameSettings::~GameSettings(){
  //Destructor no longer saves, to prevent conflicts when MtgSets::SetsList == NULL
  SAFE_DELETE(globalOptions);
  SAFE_DELETE(profileOptions);
  SAFE_DELETE(keypad);
}

GameOption GameSettings::invalid_option = GameOption(0);

GameOption& GameSettings::operator[](int optionID){
  GameOption * go = get(optionID);
  if(!go)
    return invalid_option;

  return *go;
}

GameOption* GameSettings::get(int optionID){
  string option_name = Options::getName(optionID);
  
  if(optionID < 0)
    return &invalid_option;
  else if(globalOptions && optionID <= Options::LAST_GLOBAL)
    return globalOptions->get(optionID);
  else if(profileOptions)
    return profileOptions->get(optionID);

  return &invalid_option;
}


int GameSettings::save(){
  if(globalOptions)
    globalOptions->save();

  if(profileOptions){
    //Force our directories to exist.
    MAKEDIR(RESPATH"/profiles");
    string temp = profileFile("","",false,false);
    MAKEDIR(temp.c_str()); 
    temp+="/stats";
    MAKEDIR(temp.c_str()); 
    temp = profileFile(PLAYER_SETTINGS,"",false);

    profileOptions->save();
  }

  checkProfile();

  return 1;
}

string GameSettings::profileFile(string filename, string fallback,bool sanity, bool relative)
{
  char buf[512];
  string profile = (*this)[Options::ACTIVE_PROFILE].str;

  if(!(*this)[Options::ACTIVE_PROFILE].isDefault())  {
     //No file, return root of profile directory
     if(filename == ""){ 
       sprintf(buf,"%sprofiles/%s",( relative ? "" : RESPATH"/" ),profile.c_str());
       return buf;
     }
     //Return file
     sprintf(buf,RESPATH"/profiles/%s/%s",profile.c_str(),filename.c_str());
     if(fileExists(buf)){
        if(relative)
          sprintf(buf,"profiles/%s/%s",profile.c_str(),filename.c_str());
        return buf;
     }
  }
  else{
    //Use the default directory.
    sprintf(buf,"%splayer%s%s",(relative ? "" : RESPATH"/"),(filename == "" ? "" : "/"), filename.c_str());
    return buf;
  }
  
  //Don't fallback if sanity checking is disabled..
  if(!sanity){
    sprintf(buf,"%sprofiles/%s%s%s",(relative ? "" : RESPATH"/"),profile.c_str(),(filename == "" ? "" : "/"), filename.c_str());
    return buf;
  }

  //No fallback directory. This is often a crash.
  if(fallback == "")
      return "";

  sprintf(buf,"%s%s%s%s",(relative ? "" : RESPATH"/"),fallback.c_str(),(filename == "" ? "" : "/"), filename.c_str());
  return buf;
}

void GameSettings::reloadProfile(bool images){
    SAFE_DELETE(profileOptions);
    checkProfile();
    if(images)
      resources.Refresh(); //Update images
}

void GameSettings::checkProfile(){
    if(!globalOptions)
      globalOptions = NEW GameOptions(GLOBAL_SETTINGS);

    //If it doesn't exist, load current profile.
    if(!profileOptions)
      profileOptions = NEW GameOptions(profileFile(PLAYER_SETTINGS,"",false));

    //Validation of collection, etc, only happens if the game is up.
    if(theGame == NULL || theGame->collection == NULL)
      return; 

    string pcFile = profileFile(PLAYER_COLLECTION,"",false);
    if(!pcFile.size() || !fileExists(pcFile.c_str()))
    {
      //If we had any default settings, we'd set them here.
      
      //Find the set for which we have the most variety
      int setId = 0;
      int maxcards = 0;
      for (int i=0; i< MtgSets::SetsList->nb_items; i++){
        int value = theGame->collection->countBySet(i);
        if (value > maxcards){
          maxcards = value;
          setId = i;
        }
      }

      //Make the proper directories
      if(profileOptions){
        //Force our directories to exist.
        MAKEDIR(RESPATH"/profiles");
        string temp = profileFile("","",false,false);
        MAKEDIR(temp.c_str()); 
        temp+="/stats";
        MAKEDIR(temp.c_str()); 
        temp = profileFile(PLAYER_SETTINGS,"",false);

        profileOptions->save();
      }

      //Save this set as "unlocked"
      (*profileOptions)[Options::optionSet(setId)]=1;
      profileOptions->save();

      //Give the player their first deck
      createUsersFirstDeck(setId);
    }
}

void GameSettings::createUsersFirstDeck(int setId){
#if defined (WIN32) || defined (LINUX)
  char buf[4096];
  sprintf(buf, "setID: %i", setId);
  OutputDebugString(buf);
#endif

  if(theGame == NULL || theGame->collection == NULL)
    return;

  MTGDeck *mCollection = NEW MTGDeck(options.profileFile(PLAYER_COLLECTION,"",false).c_str(), theGame->collection);
  //10 lands of each
  int sets[] = {setId};
  if (!mCollection->addRandomCards(10, sets,1, Constants::RARITY_L,"Forest")){
    mCollection->addRandomCards(10, 0,0,Constants::RARITY_L,"Forest");
  }
  if (!mCollection->addRandomCards(10, sets,1,Constants::RARITY_L,"Plains")){
    mCollection->addRandomCards(10, 0,0,Constants::RARITY_L,"Plains");
  }
  if (!mCollection->addRandomCards(10, sets,1,Constants::RARITY_L,"Swamp")){
    mCollection->addRandomCards(10, 0,0,Constants::RARITY_L,"Swamp");
  }
  if (!mCollection->addRandomCards(10, sets,1,Constants::RARITY_L,"Mountain")){
    mCollection->addRandomCards(10, 0,0,Constants::RARITY_L,"Mountain");
  }
  if (!mCollection->addRandomCards(10, sets,1,Constants::RARITY_L,"Island")){
    mCollection->addRandomCards(10, 0,0,Constants::RARITY_L,"Island");
  }


#if defined (WIN32) || defined (LINUX)
  OutputDebugString("1\n");
#endif

  //Starter Deck
  mCollection->addRandomCards(3, sets,1,Constants::RARITY_R,NULL);
  mCollection->addRandomCards(9, sets,1,Constants::RARITY_U,NULL);
  mCollection->addRandomCards(48, sets,1,Constants::RARITY_C,NULL);

#if defined (WIN32) || defined (LINUX)
  OutputDebugString("2\n");
#endif
  //Boosters
  for (int i = 0; i< 2; i++){
    mCollection->addRandomCards(1, sets,1,Constants::RARITY_R);
    mCollection->addRandomCards(3, sets,1,Constants::RARITY_U);
    mCollection->addRandomCards(11, sets,1,Constants::RARITY_C);
  }
  mCollection->save();
  SAFE_DELETE(mCollection);
}
void GameSettings::keypadTitle(string set){
  if(keypad != NULL)
    keypad->title = set;
}
SimplePad * GameSettings::keypadStart(string input, string * _dest,bool _cancel, bool _numpad, int _x,int _y ){
  if(keypad == NULL)
    keypad = NEW SimplePad();
  keypad->bShowCancel = _cancel;
  keypad->bShowNumpad = _numpad;
  keypad->mX = _x;
  keypad->mY = _y;
  keypad->Start(input,_dest);
  return keypad;
}

string GameSettings::keypadFinish(){
  if(keypad == NULL)
    return "";
  return keypad->Finish();
}

void GameSettings::keypadShutdown(){
  SAFE_DELETE(keypad);
}

int EnumDefinition::findIndex(int value){
  vector<assoc>::iterator it;
  for(it = values.begin();it!=values.end();it++){
    if(it->first == value)
      return it - values.begin();
  }

  return INVALID_ID; //Failed!
}

string GameOptionEnum::menuStr(){
  if(def){
    int idx = def->findIndex(number);
    if(idx != INVALID_ID)
      return def->values[idx].second;
  }

  char buf[32];
  sprintf(buf,"%d",number);
  return buf;
}

bool GameOptionEnum::write(std::ofstream * file, string name){
  if(!file || !def || number < 0 || number >= (int) def->values.size())
   return false;

  char writer[1024];
  sprintf(writer,"%s=%s\n", name.c_str(), menuStr().c_str());

  (*file)<<writer;
  return true;
}

bool GameOptionEnum::read(string input){
  if(!def) 
    return false;

  std::transform(input.begin(),input.end(),input.begin(),::tolower);

  vector<EnumDefinition::assoc>::iterator it;
  for(it=def->values.begin();it != def->values.end();it++){
    if(it->second == input){
      number = it->first;
      return true;
    }
  }
 
  return false;
}
OptionClosedHand OptionClosedHand::mDef;
OptionClosedHand::OptionClosedHand(){  
    mDef.values.push_back(EnumDefinition::assoc(INVISIBLE, "invisible"));
    mDef.values.push_back(EnumDefinition::assoc(VISIBLE, "visible"));
};

OptionHandDirection OptionHandDirection::mDef;
OptionHandDirection::OptionHandDirection(){      
    mDef.values.push_back(EnumDefinition::assoc(VERTICAL, "vertical"));
    mDef.values.push_back(EnumDefinition::assoc(HORIZONTAL, "horizontal"));
};
OptionManaDisplay OptionManaDisplay::mDef;
OptionManaDisplay::OptionManaDisplay(){  
    mDef.values.push_back(EnumDefinition::assoc(STATIC, "Simple"));
    mDef.values.push_back(EnumDefinition::assoc(DYNAMIC, "Eye candy"));
    mDef.values.push_back(EnumDefinition::assoc(BOTH, "Both"));
};
OptionVolume OptionVolume::mDef;
OptionVolume::OptionVolume(){  
    mDef.values.push_back(EnumDefinition::assoc(MUTE, "Mute"));
    mDef.values.push_back(EnumDefinition::assoc(MAX, "Max"));
};
OptionDifficulty OptionDifficulty::mDef;
OptionDifficulty::OptionDifficulty(){  
    mDef.values.push_back(EnumDefinition::assoc(NORMAL, "Normal"));
    mDef.values.push_back(EnumDefinition::assoc(HARDER, "Harder"));
    mDef.values.push_back(EnumDefinition::assoc(HARD, "Hard"));
    mDef.values.push_back(EnumDefinition::assoc(EVIL, "Evil"));
};