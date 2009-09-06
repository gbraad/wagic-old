#include "../include/config.h"
#include "../include/MTGGameZones.h"
#include "../include/Player.h"
#include "../include/GameOptions.h"
#include "../include/WEvent.h"
#include "../include/MTGDeck.h"

#if defined (WIN32) || defined (LINUX)
#include <time.h>
#endif

//------------------------------
//Players Game
//------------------------------

MTGPlayerCards::MTGPlayerCards(MTGAllCards * _collection, int * idList, int idListSize){
  init();
  int i;
  collection = _collection;
  for (i=0;i<idListSize;i++){
    MTGCard * card =  collection->getCardById(idList[i]);
    if (card){
      MTGCardInstance * newCard = NEW MTGCardInstance(card, this);
      library->addCard(newCard);
    }
  }
}

MTGPlayerCards::MTGPlayerCards(MTGAllCards * _collection, MTGDeck * deck){
  init();
  collection = _collection;
  map<int,int>::iterator it;
  for (it = deck->cards.begin(); it!=deck->cards.end(); it++){
    MTGCard * card = deck->getCardById(it->first);
    if (card){
      for (int i = 0; i < it->second; i++){
        MTGCardInstance * newCard = NEW MTGCardInstance(card, this);
        library->addCard(newCard);
      }
    }
  }
}

MTGPlayerCards::~MTGPlayerCards(){
  SAFE_DELETE(library);
  SAFE_DELETE(graveyard);
  SAFE_DELETE(hand);
  SAFE_DELETE(inPlay);
  SAFE_DELETE(stack);
  SAFE_DELETE(removedFromGame);
  SAFE_DELETE(garbage);
}

void MTGPlayerCards::setOwner(Player * player){
  library->setOwner(player);
  graveyard->setOwner(player);
  hand->setOwner(player);
  inPlay->setOwner(player);
  removedFromGame->setOwner(player);
  stack->setOwner(player);
  garbage->setOwner(player);
}

void MTGPlayerCards::initGame(int shuffle, int draw){
  if (shuffle) library->shuffle();
  if (draw){
    for (int i=0;i<7;i++){
      drawFromLibrary();
    }
  }
}

void MTGPlayerCards::drawFromLibrary(){
  MTGCardInstance * drownCard = library->draw();
  if(drownCard){
    hand->addCard(drownCard);
    GameObserver *g = GameObserver::GetInstance();
    WEvent * e = NEW WEventZoneChange(drownCard,library,hand);
    g->receiveEvent(e);
    //delete e;
  }
}

void MTGPlayerCards::init(){
  library = NEW MTGLibrary();
  graveyard = NEW MTGGraveyard();
  hand = NEW MTGHand();
  inPlay = NEW MTGInPlay();
  battlefield=inPlay;

  stack = NEW MTGStack();
  removedFromGame = NEW MTGRemovedFromGame();
  exile = removedFromGame;
  garbage = NEW MTGGameZone();
}


void MTGPlayerCards::showHand(){
  hand->debugPrint();
}


MTGCardInstance * MTGPlayerCards::putInPlay(MTGCardInstance * card){
  MTGGameZone * from = hand;
  MTGCardInstance * copy = hand->removeCard(card);
  if(!copy){
    copy = stack->removeCard(card); //Which one is it ???
    from = stack;
  }
  inPlay->addCard(copy);
  copy->summoningSickness = 1;
  copy->changedZoneRecently = 1.f;

  GameObserver *g = GameObserver::GetInstance();
  WEvent * e = NEW WEventZoneChange(copy, from, inPlay);
  g->receiveEvent(e);
  //delete e;

  return copy;
}

MTGCardInstance * MTGPlayerCards::putInGraveyard(MTGCardInstance * card){
  MTGCardInstance * copy = NULL;
  MTGGraveyard * grave = card->owner->game->graveyard;
  if (inPlay->hasCard(card)){
    copy = putInZone(card,inPlay, grave);
  }else if (stack->hasCard(card)){
    copy = putInZone(card,stack, grave);
  }else{
    copy = putInZone(card,hand, grave);
  }
  return copy;

}

MTGCardInstance * MTGPlayerCards::putInZone(MTGCardInstance * card, MTGGameZone * from, MTGGameZone * to){
  MTGCardInstance * copy = NULL;
  GameObserver *g = GameObserver::GetInstance();
  if (!from || !to) return card; //Error check

  int doCopy = 1;
  //When a card is moved from inPlay to inPlay (controller change, for example), it is still the same object
  if ((to == g->players[0]->game->inPlay || to == g->players[1]->game->inPlay) &&
    (from == g->players[0]->game->inPlay || from == g->players[1]->game->inPlay)){
      doCopy = 0;
  }

  if ((copy = from->removeCard(card,doCopy))){
    if (options[Options::SFXVOLUME].number > 0){
      if (to == g->players[0]->game->graveyard || to == g->players[1]->game->graveyard){
        if (card->isCreature()){
          JSample * sample = resources.RetrieveSample("graveyard.wav");
          if (sample) JSoundSystem::GetInstance()->PlaySample(sample);
        }
      }
    }

    MTGCardInstance * ret = copy;
    if (card->isToken){
      if (to != g->players[0]->game->inPlay && to != g->players[1]->game->inPlay){
        to = garbage;
        ret = NULL;
      }
    }

    to->addCard(copy);
    copy->changedZoneRecently = 1.f;
    GameObserver *g = GameObserver::GetInstance();
    WEvent * e = NEW WEventZoneChange(copy, from, to);
    g->receiveEvent(e);
    //delete e;
    return ret;
  }
  return card; //Error
}

void MTGPlayerCards::discardRandom(MTGGameZone * from){
  if (!from->nb_cards)
    return;
  int r = rand() % (from->nb_cards);
  putInZone(from->cards[r],from, graveyard);
}

int MTGPlayerCards::isInPlay(MTGCardInstance * card){
  if (inPlay->hasCard(card)){
    return 1;
  }
  return 0;
}

//--------------------------------------
// Zones specific code
//--------------------------------------

MTGGameZone::MTGGameZone() : nb_cards(0), lastCardDrawn(NULL), needShuffle(false) {
}

MTGGameZone::~MTGGameZone(){
  for (int i=0; i<nb_cards; i++) {
    delete cards[i];
  }
}

void MTGGameZone::setOwner(Player * player){
  for (int i=0; i<nb_cards; i++) {
    cards[i]->owner = player;
  }
  owner = player;
}

MTGCardInstance * MTGGameZone::removeCard(MTGCardInstance * card, int createCopy){
  int i;
  cardsMap.erase(card);
  for (i=0; i<(nb_cards); i++) {
    if (cards[i] == card){
      nb_cards--;
      cards.erase(cards.begin()+i);
	    MTGCardInstance * copy = card;
      if (card->isToken) //TODO better than this ?
        return card;
      //card->lastController = card->controller();
      if (createCopy) {
		    copy = NEW MTGCardInstance(card->model,card->owner->game);
		    copy->previous = card;
		    copy->view = card->view;
		    card->next = copy;
	    }
      copy->previousZone = this;
      return copy;
    }
  }
  return NULL;

}

MTGCardInstance * MTGGameZone::hasCard(MTGCardInstance * card){
  if (cardsMap.find(card) != cardsMap.end()) return card;
  return NULL;

}

int MTGGameZone::countByType(const char * value){
  int result = 0 ;
  for (int i=0; i<(nb_cards); i++) {
    if (cards[i]->hasType(value)){
      result++;
    }
  }
  return result;

}

int MTGGameZone::hasType(const char * value){
  for (int i=0; i<(nb_cards); i++) {
    if (cards[i]->hasType(value)){
      return 1;
    }
  }
  return 0;
}


void MTGGameZone::cleanupPhase(){
  for (int i=0; i<(nb_cards); i++)
    (cards[i])->cleanup();
}

void MTGGameZone::shuffle(){
  int i;
  for (i=0; i<(nb_cards); i++) {
    int r = i + (rand() % (nb_cards-i)); // Random remaining position.
    MTGCardInstance * temp = cards[i]; cards[i] = cards[r]; cards[r] = temp;
  }
  //srand(time(0));  // initialize seed "randomly" TODO :improve
}



void MTGGameZone::addCard(MTGCardInstance * card){
  if (!card) return;
  cards.push_back(card);
  nb_cards++;
  cardsMap[card] = 1;
  card->lastController = this->owner;

}

MTGCardInstance * MTGGameZone::draw(){
  if (!nb_cards) return NULL;
  nb_cards--;
  lastCardDrawn = cards[nb_cards];
  cards.pop_back();
  cardsMap.erase( lastCardDrawn);
  return lastCardDrawn;
}

MTGCardInstance * MTGLibrary::draw(){
  if (!nb_cards) {
    GameObserver::GetInstance()->gameOver = this->owner;
  }
  return MTGGameZone::draw();
}

void MTGGameZone::debugPrint(){
  for (int i = 0; i < nb_cards; i++)
    std::cerr << cards[i]->getName() << endl;
}





//------------------------------
int MTGInPlay::nbDefensers( MTGCardInstance * attacker){
  int result = 0;
  MTGCardInstance * defenser = getNextDefenser(NULL, attacker);
  while (defenser){
    result++;
    defenser = getNextDefenser(defenser, attacker);
  }
  return result;
}

//Return the number of creatures this card is banded with
//Number of creatures in the band is n+1 !!!
int MTGInPlay::nbPartners(MTGCardInstance * attacker){
  int result = 0;
  if (!attacker->banding) return 0;
  for (int i = 0; i < nb_cards; i ++){
    if (cards[i]->banding == attacker->banding) result++;
  }
  return result;
}

MTGCardInstance *  MTGInPlay::getNextDefenser(MTGCardInstance * previous, MTGCardInstance * attacker){
  return attacker->getNextDefenser(previous);
}

MTGCardInstance *  MTGInPlay::getNextAttacker(MTGCardInstance * previous){
  int foundprevious = 0;
  if (previous == NULL){
    foundprevious = 1;
  }
  for (int i = 0; i < nb_cards; i ++){
    MTGCardInstance * current = cards[i];
    if (current == previous){
      foundprevious = 1;
    }else if (foundprevious && current->isAttacker()){
      return current;
    }
  }
  return NULL;
}

void MTGInPlay::untapAll(){
  int i;
  for (i = 0; i < nb_cards; i ++){
    MTGCardInstance * card = cards[i];
    card->setUntapping();
    if (!card->basicAbilities[Constants::DOESNOTUNTAP] && card->getUntapBlockers()->isEmpty()){
      card->attemptUntap();
    }
  }
}


//--------------------------
void MTGLibrary::shuffleTopToBottom(int nbcards){
  if (nbcards>nb_cards) nbcards = nb_cards;
  MTGCardInstance * _cards[MTG_MAX_PLAYER_CARDS];
  for (int i= nb_cards-nbcards; i<(nb_cards); i++) {
    int r = i + (rand() % (nbcards-i)); // Random remaining position.
    MTGCardInstance * temp = cards[i]; cards[i] = cards[r]; cards[r] = temp;
  }
  for (int i= 0; i < nbcards; i++){
    _cards[i] = cards[nb_cards - 1 - i];
  }
  for (int i = nbcards; i < nb_cards; i++){
    _cards[i] = cards[i - nb_cards];
  }
  for (int i=0 ; i < nb_cards; i++){
    cards[i] = _cards[i];
  }
}


MTGGameZone * MTGGameZone::intToZone(int zoneId, MTGCardInstance * source,MTGCardInstance * target){
  Player *p, *p2;
  GameObserver * g = GameObserver::GetInstance();
  if (!source) p = g->currentlyActing();
  else p = source->controller();
  if (!target){
    p2 = p;
    target = source;//hack ?
  }
  else p2 = target->controller();

  switch(zoneId){
    case MY_GRAVEYARD: return p->game->graveyard;
    case OPPONENT_GRAVEYARD: return p->opponent()->game->graveyard;
    case TARGET_OWNER_GRAVEYARD : return target->owner->game->graveyard;
    case TARGET_CONTROLLER_GRAVEYARD:  return p2->game->graveyard;
    case GRAVEYARD : return target->owner->game->graveyard;
    case OWNER_GRAVEYARD : return target->owner->game->graveyard;

    case MY_BATTLEFIELD : return p->game->inPlay;
    case OPPONENT_BATTLEFIELD : return p->opponent()->game->inPlay;
    case TARGET_OWNER_BATTLEFIELD : return target->owner->game->inPlay;
    case TARGET_CONTROLLER_BATTLEFIELD : return p2->game->inPlay;
    case BATTLEFIELD : return p->game->inPlay;
    case OWNER_BATTLEFIELD : return target->owner->game->inPlay;

    case MY_HAND : return p->game->hand;
    case OPPONENT_HAND : return p->opponent()->game->hand;
    case TARGET_OWNER_HAND : return target->owner->game->hand;
    case TARGET_CONTROLLER_HAND : return p2->game->hand; 
    case HAND : return target->owner->game->hand;
    case OWNER_HAND : return target->owner->game->hand;

    case MY_EXILE : return p->game->removedFromGame;
    case OPPONENT_EXILE : return p->opponent()->game->removedFromGame;
    case TARGET_OWNER_EXILE :  return target->owner->game->removedFromGame;
    case TARGET_CONTROLLER_EXILE : return p2->game->removedFromGame;
    case EXILE : return target->owner->game->removedFromGame;
    case OWNER_EXILE : return target->owner->game->removedFromGame;

    case MY_LIBRARY : return p->game->library;
    case OPPONENT_LIBRARY : return p->opponent()->game->library;
    case TARGET_OWNER_LIBRARY : return target->owner->game->library;
    case TARGET_CONTROLLER_LIBRARY : return p2->game->library;
    case LIBRARY : return p->game->library;
    case OWNER_LIBRARY: return target->owner->game->library;

    case MY_STACK : return p->game->stack;
    case OPPONENT_STACK : return p->opponent()->game->stack;
    case TARGET_OWNER_STACK : return target->owner->game->stack;
    case TARGET_CONTROLLER_STACK : return p2->game->stack;
    case STACK : return p->game->stack;
    case OWNER_STACK: return target->owner->game->stack;
    default:
      return NULL;
  }
}

int MTGGameZone::zoneStringToId(string zoneName){
  const char * strings[] = {
    "mygraveyard",
    "opponentgraveyard",
    "targetownergraveyard",
    "targetcontrollergraveyard",
    "ownergraveyard",
    "graveyard",

    "myinplay",
    "opponentinplay",
    "targetownerinplay",
    "targetcontrollerinplay",
    "ownerinplay",
    "inplay",

    "mybattlefield",
    "opponentbattlefield",
    "targetownerbattlefield",
    "targetcontrollerbattlefield",
    "ownerbattlefield",
    "battlefield",

    "myhand",
    "opponenthand",
    "targetownerhand",
    "targetcontrollerhand",
    "ownerhand",
    "hand",

    "mylibrary",
    "opponentlibrary",
    "targetownerlibrary",
    "targetcontrollerlibrary",
    "ownerlibrary",
    "library",

    "myremovedfromgame",
    "opponentremovedfromgame",
    "targetownerremovedfromgame",
    "targetcontrollerremovedfromgame",
    "ownerremovedfromgame",
    "removedfromgame",

    "myexile",
    "opponentexile",
    "targetownerexile",
    "targetcontrollerexile",
    "ownerexile",
    "exile",

    "mystack",
    "opponentstack",
    "targetownerstack",
    "targetcontrollerstack",
    "ownerstack",
    "stack",

  };

  int values[] = {
    MY_GRAVEYARD,
    OPPONENT_GRAVEYARD,
    TARGET_OWNER_GRAVEYARD ,
    TARGET_CONTROLLER_GRAVEYARD,
    OWNER_GRAVEYARD ,
    GRAVEYARD,

    MY_BATTLEFIELD,
    OPPONENT_BATTLEFIELD,
    TARGET_OWNER_BATTLEFIELD ,
    TARGET_CONTROLLER_BATTLEFIELD,
    OWNER_BATTLEFIELD ,
    BATTLEFIELD,

    MY_BATTLEFIELD,
    OPPONENT_BATTLEFIELD,
    TARGET_OWNER_BATTLEFIELD ,
    TARGET_CONTROLLER_BATTLEFIELD,
    OWNER_BATTLEFIELD ,
    BATTLEFIELD,

    MY_HAND,
    OPPONENT_HAND,
    TARGET_OWNER_HAND ,
    TARGET_CONTROLLER_HAND,
    OWNER_HAND ,
    HAND,

    MY_LIBRARY,
    OPPONENT_LIBRARY,
    TARGET_OWNER_LIBRARY ,
    TARGET_CONTROLLER_LIBRARY,
    OWNER_LIBRARY ,
    LIBRARY,

    MY_EXILE,
    OPPONENT_EXILE,
    TARGET_OWNER_EXILE ,
    TARGET_CONTROLLER_EXILE,
    OWNER_EXILE ,
    EXILE,

    MY_EXILE,
    OPPONENT_EXILE,
    TARGET_OWNER_EXILE ,
    TARGET_CONTROLLER_EXILE,
    OWNER_EXILE ,
    EXILE,

    MY_STACK,
    OPPONENT_STACK,
    TARGET_OWNER_STACK ,
    TARGET_CONTROLLER_STACK,
    OWNER_STACK ,
    STACK,
  };

  int max = sizeof(values) / sizeof*(values);

  for (int i = 0; i < max; ++i){
    if(zoneName.compare(strings[i]) == 0){
      return values[i];
    }
  }
  return 0;
}

MTGGameZone * MTGGameZone::stringToZone(string zoneName, MTGCardInstance * source,MTGCardInstance * target){
  return intToZone(zoneStringToId(zoneName), source,target);
}

ostream& MTGGameZone::toString(ostream& out) const { return out << "Unknown zone"; }
ostream& MTGLibrary::toString(ostream& out) const { return out << "Library " << *owner; }
ostream& MTGGraveyard::toString(ostream& out) const { return out << "Graveyard " << *owner; }
ostream& MTGHand::toString(ostream& out) const { return out << "Hand " << *owner; }
ostream& MTGRemovedFromGame::toString(ostream& out) const { return out << "RemovedFromGame " << *owner; }
ostream& MTGStack::toString(ostream& out) const { return out << "Stack " << *owner; }
ostream& MTGInPlay::toString(ostream& out) const { return out << "InPlay " << *owner; }
ostream& operator<<(ostream& out, const MTGGameZone& z)
{
  return z.toString(out);
}
