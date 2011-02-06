#include "PrecompiledHeader.h"
#include "AllAbilities.h"

//Activated Abilities

//Generic Activated Abilities
GenericActivatedAbility::GenericActivatedAbility(int _id, MTGCardInstance * card, MTGAbility * a, ManaCost * _cost, int _tap,
        int limit, int restrictions, MTGGameZone * dest) :
    ActivatedAbility(_id, card, _cost, restrictions, _tap), NestedAbility(a), limitPerTurn(limit), activeZone(dest)
{
    counters = 0;
    target = ability->target;
}

int GenericActivatedAbility::resolve()
{
    counters++;
    ManaCost * diff = abilityCost->Diff(cost);
    source->X = diff->hasX();
    SAFE_DELETE(diff);
    //SAFE_DELETE(abilityCost); this line has been reported as a bug. removing it doesn't seem to break anything, although I didn't get any error in the test suite by leaving it either, so... leaving it for now as a comment, in case.
    ability->target = target; //may have been updated...
    if (ability)
        return ability->resolve();
    return 0;
}

const char * GenericActivatedAbility::getMenuText()
{
    if (ability)
        return ability->getMenuText();
    return "Error";
}

int GenericActivatedAbility::isReactingToClick(MTGCardInstance * card, ManaCost * mana)
{
    if (dynamic_cast<AAMorph*> (ability) && !card->isMorphed && !card->morphed && card->turningOver)
        return 0;
    if (limitPerTurn && counters >= limitPerTurn)
        return 0;
    return ActivatedAbility::isReactingToClick(card, mana);
}

void GenericActivatedAbility::Update(float dt)
{
    if (newPhase != currentPhase && newPhase == Constants::MTG_PHASE_AFTER_EOT)
    {
        counters = 0;
    }
    ActivatedAbility::Update(dt);
}

int GenericActivatedAbility::testDestroy()
{
    if (!activeZone)
        return ActivatedAbility::testDestroy();
    if (activeZone->hasCard(source))
        return 0;
    return 1;

}

GenericActivatedAbility * GenericActivatedAbility::clone() const
{
    GenericActivatedAbility * a = NEW GenericActivatedAbility(*this);
    a->cost = NEW ManaCost();
    a->cost->copy(cost);
    a->ability = ability->clone();
    return a;
}

GenericActivatedAbility::~GenericActivatedAbility()
{
    SAFE_DELETE(ability);
}

//AA Alter Poison
AAAlterPoison::AAAlterPoison(int _id, MTGCardInstance * _source, Targetable * _target, int poison, ManaCost * _cost, int doTap,
        int who) :
    ActivatedAbilityTP(_id, _source, _target, _cost, doTap, who), poison(poison)
{
}

int AAAlterPoison::resolve()
{
    Damageable * _target = (Damageable *) getTarget();
    if (_target)
    {
        _target->poisonCount += poison;
    }
    return 0;
}

const char * AAAlterPoison::getMenuText()
{
    return "Poison";
}

AAAlterPoison * AAAlterPoison::clone() const
{
    AAAlterPoison * a = NEW AAAlterPoison(*this);
    a->isClone = 1;
    return a;
}

AAAlterPoison::~AAAlterPoison()
{
}

//Damage Prevent
AADamagePrevent::AADamagePrevent(int _id, MTGCardInstance * _source, Targetable * _target, int preventing, ManaCost * _cost,
        int doTap, int who) :
    ActivatedAbilityTP(_id, _source, _target, _cost, doTap, who), preventing(preventing)
{
    aType = MTGAbility::STANDARD_PREVENT;
}

int AADamagePrevent::resolve()
{
    Damageable * _target = (Damageable *) getTarget();
    if (_target)
    {
        _target->preventable += preventing;
    }
    return 0;
}

const char * AADamagePrevent::getMenuText()
{
    return "Prevent Damage";
}

AADamagePrevent * AADamagePrevent::clone() const
{
    AADamagePrevent * a = NEW AADamagePrevent(*this);
    a->isClone = 1;
    return a;
}

AADamagePrevent::~AADamagePrevent()
{
}

//AADamager
AADamager::AADamager(int _id, MTGCardInstance * _source, Targetable * _target, string d, ManaCost * _cost, int doTap,
        int who) :
    ActivatedAbilityTP(_id, _source, _target, _cost, doTap, who), d(d)
{
    aType = MTGAbility::DAMAGER;
    }

    int AADamager::resolve()
    {
        Damageable * _target = (Damageable *) getTarget();
        if (_target)
        {
            WParsedInt damage(d, NULL, (MTGCardInstance *)source);
            game->mLayers->stackLayer()->addDamage(source, _target, damage.getValue());
            game->mLayers->stackLayer()->resolve();
            return 1;
        }
        return 0;
    }

    int AADamager::getDamage()
    {
        WParsedInt damage(d, NULL, (MTGCardInstance *)source);
        return damage.getValue();
    }

    const char * AADamager::getMenuText()
    {
        return "Damage";
    }

AADamager * AADamager::clone() const
{
    AADamager * a = NEW AADamager(*this);
    a->isClone = 1;
    return a;
}


//AADepleter
AADepleter::AADepleter(int _id, MTGCardInstance * card, Targetable * _target,string nbcardsStr, ManaCost * _cost, int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who),nbcardsStr(nbcardsStr)
{

}
    int AADepleter::resolve()
    {

        Targetable * _target = getTarget();
        Player * player;
        if (_target)
        {
            WParsedInt numCards(nbcardsStr, NULL, source);
            if (_target->typeAsTarget() == TARGET_CARD)
            {
                player = ((MTGCardInstance *) _target)->controller();
            }
            else
            {
                player = (Player *) _target;
            }
            MTGLibrary * library = player->game->library;
            for (int i = 0; i < numCards.getValue(); i++)
            {
                if (library->nb_cards)
                    player->game->putInZone(library->cards[library->nb_cards - 1], library, player->game->graveyard);
            }
        }
        return 1;
    }

const char * AADepleter::getMenuText()
{
    return "Deplete";
}

AADepleter * AADepleter::clone() const
{
    AADepleter * a = NEW AADepleter(*this);
    a->isClone = 1;
    return a;
}

//AACopier
AACopier::AACopier(int _id, MTGCardInstance * _source, MTGCardInstance * _target, ManaCost * _cost) :
    ActivatedAbility(_id, _source, _cost, 0, 0)
{
    target = _target;
}

int AACopier::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        source->copy(_target);
        return 1;
    }
    return 0;
}

const char * AACopier::getMenuText()
{
    return "Copy";
}

AACopier * AACopier::clone() const
{
    AACopier * a = NEW AACopier(*this);
    a->isClone = 1;
    return a;
}

//phaser
AAPhaseOut::AAPhaseOut(int _id, MTGCardInstance * _source, MTGCardInstance * _target, ManaCost * _cost) :
    ActivatedAbility(_id, _source, _cost, 0, 0)
{
    target = _target;
}

int AAPhaseOut::resolve()
{GameObserver * g = g->GetInstance();
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        _target->isTempPhased = true;
        return 1;
    }
    return 0;
}

const char * AAPhaseOut::getMenuText()
{
    return "Phase Out";
}

AAPhaseOut * AAPhaseOut::clone() const
{
    AAPhaseOut * a = NEW AAPhaseOut(*this);
    a->isClone = 1;
    return a;
}

//Counters
AACounter::AACounter(int id, MTGCardInstance * source, MTGCardInstance * target,string counterstring, const char * _name, int power, int toughness,
        int nb, ManaCost * cost, int doTap) :
    ActivatedAbility(id, source, cost, 0, doTap),counterstring(counterstring), nb(nb), power(power), toughness(toughness), name(_name)
{
    this->target = target;
    if (name.find("Level"))
        aType = MTGAbility::STANDARD_LEVELUP;
    menu = "";
}

    int AACounter::resolve()
    {
        if (target)
        {
            MTGCardInstance * _target = (MTGCardInstance *) target;
            AbilityFactory af;
            Counter * checkcounter = af.parseCounter(counterstring, _target, NULL);
            nb = checkcounter->nb;
            delete checkcounter;
            if (nb > 0)
            {
            for (int i = 0; i < nb; i++)
            {
                while (_target->next)
                    _target = _target->next;
                _target->counters->addCounter(name.c_str(), power, toughness);
            }
        }
        else
        {
            for (int i = 0; i < -nb; i++)
            {
                while (_target->next)
                    _target = _target->next;
                _target->counters->removeCounter(name.c_str(), power, toughness);
            }
        }
        if(_target->toughness <= 0 && _target->has(Constants::INDESTRUCTIBLE) && toughness < 0)
            _target->controller()->game->putInGraveyard(_target);
        return nb;
    }
    return 0;
}

const char* AACounter::getMenuText()
{
    if (menu.size())
    {
        return menu.c_str();
    }
    char buffer[128];

    if (name.size())
    {
        string s = name;
        menu.append(s.c_str());
    }

    if (power != 0 || toughness != 0)
    {
        sprintf(buffer, " %i/%i", power, toughness);
        menu.append(buffer);
    }

    menu.append(" Counter");
    if (nb != 1 && !(nb < -1000))
    {
        sprintf(buffer, ": %i", nb);
        menu.append(buffer);
    }

    sprintf(menuText, "%s", menu.c_str());
    return menuText;
}

AACounter * AACounter::clone() const
{
    AACounter * a = NEW AACounter(*this);
    a->isClone = 1;
    return a;
}

//Counters
AARemoveAllCounter::AARemoveAllCounter(int id, MTGCardInstance * source, MTGCardInstance * target, const char * _name, int power, int toughness,
        int nb,bool all, ManaCost * cost, int doTap) :
    ActivatedAbility(id, source, cost, 0, doTap), nb(nb), power(power), toughness(toughness), name(_name),all(all)
{
    this->target = target;
    menu = "";
}

    int AARemoveAllCounter::resolve()
    {
        if (target)
        {
            MTGCardInstance * _target = (MTGCardInstance *) target;
            if (all )
            {
                for(int amount = 0;amount < _target->counters->mCount;amount++)
                {
                    while(_target->counters->counters[amount]->nb > 0)
                        _target->counters->removeCounter(_target->counters->counters[amount]->name.c_str(),_target->counters->counters[amount]->power,_target->counters->counters[amount]->toughness);

                }
            }
            Counter * targetCounter = NULL;
            if (_target->counters && _target->counters->hasCounter(name.c_str(), power, toughness))
            {
                targetCounter = _target->counters->hasCounter(name.c_str(), power, toughness);
                nb = targetCounter->nb;
            }

            if (nb > 0)
            {
                for (int i = 0; i < nb; i++)
                {
                    while (_target->next)
                        _target = _target->next;
                    _target->counters->removeCounter(name.c_str(), power, toughness);
                }
            }
        return nb;
    }
    return 0;
}

const char* AARemoveAllCounter::getMenuText()
{
    if (menu.size())
    {
        return menu.c_str();
    }
    char buffer[128];

    if (name.size())
    {
        string s = name;
        menu.append(s.c_str());
    }

    if (power != 0 || toughness != 0)
    {
        sprintf(buffer, " %i/%i", power, toughness);
        menu.append(buffer);
    }

    menu.append(" Counter Removed");
    if (nb != 1)
    {
        sprintf(buffer, ": %i", nb);
        menu.append(buffer);
    }

    sprintf(menuText, "%s", menu.c_str());
    return menuText;
}

AARemoveAllCounter * AARemoveAllCounter::clone() const
{
    AARemoveAllCounter * a = NEW AARemoveAllCounter(*this);
    a->isClone = 1;
    return a;
}

// Fizzler
AAFizzler::AAFizzler(int _id, MTGCardInstance * card, Spell * _target, ManaCost * _cost, int _tap) :
ActivatedAbility(_id, card, _cost, 0, _tap)
{
    target = _target;
}

int AAFizzler::resolve()
{
    Spell * _target = (Spell *) target;
    if(!target && !_target)
    {
        //if we hit this condiational its because Ai was targetting.
        target = source->target;
        Interruptible * laststackitem = game->mLayers->stackLayer()->getAt(-2);
        //fizzle the spell that was played directly before this counterspell
        //ai will tend to respond to you extremely quick.
        if (laststackitem && laststackitem->type == ACTION_SPELL)
        {
            Spell * spell = (Spell*) laststackitem;
            _target = spell;
        }
    }
    if (target && _target->source->has(Constants::NOFIZZLE))
        return 0;
    game->mLayers->stackLayer()->Fizzle(_target);
    return 1;
}

const char * AAFizzler::getMenuText()
{
    return "Fizzle";
}

AAFizzler* AAFizzler::clone() const
{
    AAFizzler * a = NEW AAFizzler(*this);
    a->isClone = 1;
    return a;
}
// BanishCard implementations

AABanishCard::AABanishCard(int _id, MTGCardInstance * _source, MTGCardInstance * _target, int _banishmentType) :
    ActivatedAbility(_id, _source, NULL), banishmentType(_banishmentType)
{
    if (_target)
        target = _target;
}

const char * AABanishCard::getMenuText()
{
    return "Send to graveyard";
}

int AABanishCard::resolve()
{
    DebugTrace("This is not implemented!");
    return 0;
}

AABanishCard * AABanishCard::clone() const
{
    AABanishCard * a = NEW AABanishCard(*this);
    a->isClone = 1;
    return a;
}

// Bury

AABuryCard::AABuryCard(int _id, MTGCardInstance * _source, MTGCardInstance * _target, int _banishmentType) :
    AABanishCard(_id, _source, _target, AABanishCard::BURY)
{
}

int AABuryCard::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        return _target->bury();
    }
    return 0;
}

const char * AABuryCard::getMenuText()
{
    return "Bury";
}

AABuryCard * AABuryCard::clone() const
{
    AABuryCard * a = NEW AABuryCard(*this);
    a->isClone = 1;
    return a;
}

// Destroy

AADestroyCard::AADestroyCard(int _id, MTGCardInstance * _source, MTGCardInstance * _target, int _banishmentType) :
    AABanishCard(_id, _source, _target, AABanishCard::DESTROY)
{
}

int AADestroyCard::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        return _target->destroy();
    }
    return 0;
}

const char * AADestroyCard::getMenuText()
{
    return "Destroy";
}

AADestroyCard * AADestroyCard::clone() const
{
    AADestroyCard * a = NEW AADestroyCard(*this);
    a->isClone = 1;
    return a;
}

// Sacrifice
AASacrificeCard::AASacrificeCard(int _id, MTGCardInstance * _source, MTGCardInstance * _target, int _banishmentType) :
    AABanishCard(_id, _source, _target, AABanishCard::SACRIFICE)
{
}

int AASacrificeCard::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        Player * p = _target->controller();
        WEvent * e = NEW WEventCardSacrifice(_target);
        GameObserver * game = GameObserver::GetInstance();
        game->receiveEvent(e);
        p->game->putInGraveyard(_target);
        return 1;
    }
    return 0;
}

const char * AASacrificeCard::getMenuText()
{
    return "Sacrifice";
}

AASacrificeCard * AASacrificeCard::clone() const
{
    AASacrificeCard * a = NEW AASacrificeCard(*this);
    a->isClone = 1;
    return a;
}

// Discard 

AADiscardCard::AADiscardCard(int _id, MTGCardInstance * _source, MTGCardInstance * _target, int _banishmentType) :
    AABanishCard(_id, _source, _target, AABanishCard::DISCARD)
{
}

int AADiscardCard::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        Player * p = _target->controller();
        WEvent * e = NEW WEventCardDiscard(_target);
        GameObserver * game = GameObserver::GetInstance();
        game->receiveEvent(e);
        p->game->putInGraveyard(_target);
        return 1;
    }
    return 0;
}

const char * AADiscardCard::getMenuText()
{
    return "Discard";
}

AADiscardCard * AADiscardCard::clone() const
{
    AADiscardCard * a = NEW AADiscardCard(*this);
    a->isClone = 1;
    return a;
}

AADrawer::AADrawer(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, string nbcardsStr, int _tap,
        int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who), nbcardsStr(nbcardsStr)
{
    aType = MTGAbility::STANDARD_DRAW;
}

    int AADrawer::resolve()
    {
        Targetable * _target = getTarget();
        Player * player;
        if (_target)
        {
            WParsedInt numCards(nbcardsStr, NULL, source);
            if (_target->typeAsTarget() == TARGET_CARD)
            {
                player = ((MTGCardInstance *) _target)->controller();
            }
            else
            {
                player = (Player *) _target;
            }
            game->mLayers->stackLayer()->addDraw(player, numCards.getValue());
            game->mLayers->stackLayer()->resolve();
        }
        return 1;
    }

    int AADrawer::getNumCards()
    {
        WParsedInt numCards(nbcardsStr, NULL, source);
        return numCards.getValue();
    }

const char * AADrawer::getMenuText()
{
    return "Draw";
}

AADrawer * AADrawer::clone() const
{
    AADrawer * a = NEW AADrawer(*this);
    a->isClone = 1;
    return a;
}

// AAFrozen: Prevent a card from untapping during next untap phase
AAFrozen::AAFrozen(int id, MTGCardInstance * card, MTGCardInstance * _target, ManaCost * _cost, int doTap) :
ActivatedAbility(id, card, _cost, 0, doTap)
{
    target = _target;
}

int AAFrozen::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next; //This is for cards such as rampant growth
        _target->frozen += 1;
    }
    return 1;
}

const char * AAFrozen::getMenuText()
{
    return "Freeze";
}

AAFrozen * AAFrozen::clone() const
{
    AAFrozen * a = NEW AAFrozen(*this);
    a->isClone = 1;
    return a;
}

// chose a new target for an aura or enchantment and equip it note: VERY basic right now.
AANewTarget::AANewTarget(int id, MTGCardInstance * card, MTGCardInstance * _target,bool retarget, ManaCost * _cost, int doTap) :
ActivatedAbility(id, card, _cost, 0, doTap),retarget(retarget)
{
    target = _target;
}

int AANewTarget::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if(retarget)
    {
        _target = source;
        source = (MTGCardInstance *) target;
    }
    if (_target)
    {
        while (_target->next)
            _target = _target->next; 
        _target->controller()->game->putInZone(_target, _target->currentZone,
            _target->owner->game->exile);
        _target = _target->next;

        MTGCardInstance * refreshed = source->controller()->game->putInZone(_target,_target->currentZone,source->controller()->game->battlefield);
        Spell * reUp = NEW Spell(refreshed);
        if(reUp->source->hasSubtype("aura"))
        {
            reUp->source->target = source;
            reUp->resolve();
        }
        if(_target->hasSubtype("equipment"))
        {
            reUp->resolve();
            GameObserver * g = g->GetInstance();
            for (int i = 1; i < g->mLayers->actionLayer()->mCount; i++)
            {
                MTGAbility * a = ((MTGAbility *) g->mLayers->actionLayer()->mObjects[i]);
                AEquip * eq = dynamic_cast<AEquip*> (a);
                if (eq && eq->source == reUp->source)
                {
                    ((AEquip*)a)->unequip();
                    ((AEquip*)a)->equip(source);
                }
            }
        }
        delete reUp;
        if(retarget)
        {
            target = source;
            source = _target;
        }

    }
    return 1;
}

const char * AANewTarget::getMenuText()
{
    return "New Target";
}

AANewTarget * AANewTarget::clone() const
{
    AANewTarget * a = NEW AANewTarget(*this);
    a->isClone = 1;
    a->oneShot = 1;
    return a;
}
// morph a card
AAMorph::AAMorph(int id, MTGCardInstance * card, MTGCardInstance * _target, ManaCost * _cost, int doTap) :
ActivatedAbility(id, card, _cost, restrictions, doTap)
{
    target = _target;
}

int AAMorph::resolve()
{
    MTGCardInstance * Morpher = (MTGCardInstance*)source;
    if(!Morpher->isMorphed && !Morpher->morphed && Morpher->turningOver)
        return 0;
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next; 

        AbilityFactory af;
        _target->morphed = false;
        _target->isMorphed = false;
        _target->turningOver = true;
        af.getAbilities(&currentAbilities, NULL, _target,NULL);
        for (size_t i = 0; i < currentAbilities.size(); ++i)
        {
            MTGAbility * a = currentAbilities[i];
            a->source = (MTGCardInstance *) _target;
            if( a && dynamic_cast<AAMorph *> (a))
            {
                a->removeFromGame();
                GameObserver * g = g->GetInstance();
                g->removeObserver(a);
            }
            if (a)
            {
                if (a->oneShot)
                {
                    a->resolve();
                    delete (a);
                }
                else
                {
                    a->addToGame();
                }
            }
        }
        currentAbilities.clear();
        testDestroy();
    }
    return 1;
}

int AAMorph::testDestroy()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if(target)
    {
        if(_target->turningOver && !_target->isMorphed && !_target->morphed)
        {
            GameObserver * g = g->GetInstance();
            g->removeObserver(this);
            return 1;
        }
    }
    return 0;
}

const char * AAMorph::getMenuText()
{
    return "Morph";
}

AAMorph * AAMorph::clone() const
{
    AAMorph * a = NEW AAMorph(*this);
    a->isClone = 1;
    a->forceDestroy = 1;
    return a;
}
// AADYNAMIC: dynamic ability builder
AADynamic::AADynamic(int id, MTGCardInstance * card, Damageable * _target,int type,int effect,int who,int amountsource,MTGAbility * storedAbility, ManaCost * _cost, int doTap) :
ActivatedAbility(id, card, _cost, 0, doTap),type(type),effect(effect),who(who),amountsource(amountsource),eachother(eachother),storedAbility(storedAbility)
{
    target = _target;
    sourceamount = 0;
    targetamount = 0;
    eachother = false;
    tosrc = false;
    menu = "";
    OriginalSrc = source;
    storedAbility = storedAbility;
    clonedStored = NULL;
}

int AADynamic::resolve()
{
    Damageable * _target = (Damageable *) target;
    Damageable * secondaryTarget = NULL;
    if(amountsource == 2)
        source = (MTGCardInstance * )_target;
    switch(who)
    {
    case 1://each other, both take the effect
        eachother = true;
        break;
    case 2:
        source = ((MTGCardInstance *) _target);
        _target = _target;
        break;
    case 3:
        _target = _target;
        secondaryTarget = ((MTGCardInstance *) _target)->controller();
        break;
    case 4:
        _target = _target;
        secondaryTarget = ((MTGCardInstance *) _target)->controller()->opponent();
        break;
    case 5:
        tosrc = true;
        break;
    case 6:
        _target = _target;
        secondaryTarget = ((MTGCardInstance *) OriginalSrc)->controller();
        break;
    case 7:
        secondaryTarget = OriginalSrc->controller()->opponent();
        break;
    default:
        _target = _target;
        break;
    }
    if(amountsource == 3)
        _target = OriginalSrc->controller();//looking at controller for amount
    if(amountsource == 4)
        _target = OriginalSrc->controller()->opponent();//looking at controllers opponent for amount
    if(!_target)
        return 0;
    while (_target->typeAsTarget() == TARGET_CARD && ((MTGCardInstance *)_target)->next)
        _target = ((MTGCardInstance *)_target)->next;

    //find the amount variables that will be used
    sourceamount = 0;
    targetamount = 0;
    int colored = 0;
    switch(type)
    {
    case 0:
        sourceamount = ((MTGCardInstance *) source)->power;
        targetamount = ((MTGCardInstance *) _target)->power;
        if(eachother )
            sourceamount = ((MTGCardInstance *) source)->power;
        break;
    case 1:
        sourceamount = ((MTGCardInstance *) source)->toughness;
        targetamount = ((MTGCardInstance *) _target)->toughness;
        if(eachother )
            sourceamount = ((MTGCardInstance *) source)->toughness;
        break;
    case 2:
        if(amountsource == 1)
            sourceamount = ((MTGCardInstance *) source)->getManaCost()->getConvertedCost();
        else
            sourceamount = ((MTGCardInstance *) _target)->getManaCost()->getConvertedCost();
        break;
    case 3:
        for (int i = Constants::MTG_COLOR_GREEN; i <= Constants::MTG_COLOR_WHITE; ++i)
        {
            if (amountsource == 1 && ((MTGCardInstance *)source)->hasColor(i))
                ++colored;
            else
                if (amountsource == 2 && ((MTGCardInstance *)_target)->hasColor(i))
                    ++colored;
        }
        sourceamount = colored;
        break;
    case 4:
        {
            Counter * targetCounter = NULL;
            if(amountsource == 2)
            {
                if (((MTGCardInstance *)_target)->counters && ((MTGCardInstance *)_target)->counters->hasCounter("age", 0, 0))
                {
                    targetCounter = ((MTGCardInstance *)_target)->counters->hasCounter("age", 0, 0);
                    sourceamount = targetCounter->nb;
                }
            }
            else
            {
                if (((MTGCardInstance *)source)->counters && ((MTGCardInstance *)source)->counters->hasCounter("age", 0, 0))
                {
                    targetCounter = ((MTGCardInstance *)source)->counters->hasCounter("age", 0, 0);
                    sourceamount = targetCounter->nb;
                }
            }
            break;
        }
    case 5:
        {
            Counter * targetCounter = NULL;
            if(amountsource == 2)
            {
                if (((MTGCardInstance *)_target)->counters && ((MTGCardInstance *)_target)->counters->hasCounter("charge", 0, 0))
                {
                    targetCounter = ((MTGCardInstance *)_target)->counters->hasCounter("charge", 0, 0);
                    sourceamount = targetCounter->nb;
                }
            }
            else
            {
                if (((MTGCardInstance *)source)->counters && ((MTGCardInstance *)source)->counters->hasCounter("charge", 0, 0))
                {
                    targetCounter = ((MTGCardInstance *)source)->counters->hasCounter("charge", 0, 0);
                    sourceamount = targetCounter->nb;
                }
            }
            break;
        }
    case 6:
        {
            Counter * targetCounter = NULL;
            if(amountsource == 2)
            {
                if (((MTGCardInstance *)_target)->counters && ((MTGCardInstance *)_target)->counters->hasCounter(1, 1))
                {
                    targetCounter = ((MTGCardInstance *)_target)->counters->hasCounter(1,1);
                    sourceamount = targetCounter->nb;
                }
            }
            else
            {
                if (((MTGCardInstance *)source)->counters && ((MTGCardInstance *)source)->counters->hasCounter(1, 1))
                {
                    targetCounter = ((MTGCardInstance *)source)->counters->hasCounter(1,1);
                    sourceamount = targetCounter->nb;
                }
            }
            break;
        }
    case 7:
        {
            sourceamount = _target->thatmuch;
            break;
        }
    default:
        break;
    }

    if(secondaryTarget != NULL)
    {
        _target = secondaryTarget;
    }
    if (_target)
    {
        while (_target->typeAsTarget() == TARGET_CARD && ((MTGCardInstance *)_target)->next)
            _target = ((MTGCardInstance *)_target)->next;

        switch(effect)
        {
        case 0://deal damage
            if(storedAbility)
                activateStored();
            if(tosrc == false)
                game->mLayers->stackLayer()->addDamage((MTGCardInstance *)source, _target, sourceamount);
            else
                game->mLayers->stackLayer()->addDamage((MTGCardInstance *)source, OriginalSrc, sourceamount);
            if(eachother )
            {
                game->mLayers->stackLayer()->addDamage((MTGCardInstance *)_target, source, targetamount);
            }
            return 1;
            break;
        case 1://draw cards
            if(storedAbility)
                activateStored();
            game->mLayers->stackLayer()->addDraw((Player *)_target,sourceamount);
            return 1;
            break;
        case 2://gain life
            if(storedAbility)
                activateStored();
            game->mLayers->stackLayer()->addLife(_target,sourceamount);
            return 1;
            break;
        case 3://pump power
            {
                if(storedAbility)
                    activateStored();
                if(tosrc == false)
                {
                    AInstantPowerToughnessModifierUntilEOT * a = NEW AInstantPowerToughnessModifierUntilEOT(this->GetId(), source, (MTGCardInstance*)_target,NEW WParsedPT(sourceamount,0));
                    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source,(MTGCardInstance*)_target, a);
                    wrapper->addToGame();
                    return 1;
                }
                else
                {
                    AInstantPowerToughnessModifierUntilEOT * a = NEW AInstantPowerToughnessModifierUntilEOT(this->GetId(), source, OriginalSrc,NEW WParsedPT(sourceamount,0));
                    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source,OriginalSrc, a);
                    wrapper->addToGame();
                    return 1;
                }
                break;
            }
        case 4://pump toughness
            {
                if(storedAbility)
                    activateStored();
                if(tosrc == false)
                {
                    AInstantPowerToughnessModifierUntilEOT * a = NEW AInstantPowerToughnessModifierUntilEOT(this->GetId(), source, (MTGCardInstance*)_target,NEW WParsedPT(0,sourceamount));
                    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source,(MTGCardInstance*)_target, a);
                    wrapper->addToGame();
                    return 1;
                }
                else
                {
                    AInstantPowerToughnessModifierUntilEOT * a = NEW AInstantPowerToughnessModifierUntilEOT(this->GetId(), source, OriginalSrc,NEW WParsedPT(0,sourceamount));
                    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source,OriginalSrc, a);
                    wrapper->addToGame();
                    return 1;
                }
                break;
            }
        case 5://pump both
            {
                if(storedAbility)
                    activateStored();
                if(tosrc == false)
                {
                    AInstantPowerToughnessModifierUntilEOT * a = NEW AInstantPowerToughnessModifierUntilEOT(this->GetId(), source, (MTGCardInstance*)_target,NEW WParsedPT(sourceamount,sourceamount));
                    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source,(MTGCardInstance*)_target, a);
                    wrapper->addToGame();
                    return 1;
                }
                else
                {
                    AInstantPowerToughnessModifierUntilEOT * a = NEW AInstantPowerToughnessModifierUntilEOT(this->GetId(), source, OriginalSrc,NEW WParsedPT(sourceamount,sourceamount));
                    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source,OriginalSrc, a);
                    wrapper->addToGame();
                    return 1;
                }
                break;
            }
        case 6://lose life
            if(storedAbility)
                activateStored();
            game->mLayers->stackLayer()->addLife(_target,(sourceamount * -1));
            return 1;
            break;
        case 7://deplete cards
            {
                if(storedAbility)
                    activateStored();
                Player * player = (Player *)_target;
                MTGLibrary * library = player->game->library;
                for (int i = 0; i < sourceamount; i++)
                {
                    if (library->nb_cards)
                        player->game->putInZone(library->cards[library->nb_cards - 1], library, player->game->graveyard);
                }
                return 1;
                break;
            }
        case 8:
            {
                if(_target->typeAsTarget() != TARGET_CARD)
                    _target = OriginalSrc;
                for(int j = 0;j < sourceamount;j++)
                    ((MTGCardInstance*)_target)->counters->addCounter(1,1);
                break;
            }
        default:
            return 0;
        }
    }

    return 0;
}

int AADynamic::activateStored()
{
    clonedStored = storedAbility->clone();
    clonedStored->target = target;
    if (clonedStored->oneShot)
    {
        clonedStored->resolve();
        delete (clonedStored);
    }
    else
    {
        clonedStored->addToGame();
    }
    return 1;
}

const char * AADynamic::getMenuText()
{
    if (menu.size())
    {
        return menu.c_str();
    }

    switch(type)
    {
    case 0:
        menu.append("Power");
        break;
    case 1:
        menu.append("Tough");
        break;
    case 2:
        menu.append("Mana");
        break;
    case 3:
        menu.append("color");
        break;
    case 4:
        menu.append("Elder");
        break;
    case 5:
        menu.append("Charged");
        break;
    case 6:
        menu.append("Counter");
        break;
    case 7:
        menu.append("That Many ");
        break;
    default:
        break;
    }

    switch(effect)
    {
    case 0:
        menu.append("Strike");
        break;
    case 1:
        menu.append("Draw");
        break;
    case 2:
        menu.append("Life");
        break;
    case 3:
        menu.append("Pump");
        break;
    case 4:
        menu.append("Fortify");
        break;
    case 5:
        menu.append("Buff");
        break;
    case 6:
        menu.append("Drain");
        break;
    case 7:
        menu.append("Deplete!");
        break;
    case 8:
        menu.append("Counters!");
        break;
    default:
        break;
    }
    sprintf(menuText, "%s", menu.c_str());
    return menuText;
}

AADynamic * AADynamic::clone() const
{
    AADynamic * a = NEW AADynamic(*this);
    a->isClone = 1;
    return a;
}

AADynamic::~AADynamic()
{
    if (!isClone)
        SAFE_DELETE(storedAbility);
}

//AALifer
AALifer::AALifer(int _id, MTGCardInstance * card, Targetable * _target, string life_s, ManaCost * _cost, int _tap, int who) :
ActivatedAbilityTP(_id, card, _target, _cost, _tap, who),life_s(life_s)
{
    aType = MTGAbility::LIFER;
}

int AALifer::resolve()
{  
    Damageable * _target = (Damageable *) getTarget();
    if (!_target)
        return 0;

    WParsedInt life(life_s, NULL, source);
    if (_target->type_as_damageable == DAMAGEABLE_MTGCARDINSTANCE)
    {
        _target = ((MTGCardInstance *) _target)->controller();
    }
    Player *player = (Player*)_target;
    player->gainOrLoseLife(life.getValue());

    return 1;
}

int AALifer::getLife()
{
    WParsedInt life(life_s, NULL, source);
    return life.getValue();
}

const char * AALifer::getMenuText()
{
    if(getLife() < 0)
        return "Life Loss";
    return "Life";
}

AALifer * AALifer::clone() const
{
    AALifer * a = NEW AALifer(*this);
    a->isClone = 1;
    return a;
}



//Lifeset
AALifeSet::AALifeSet(int _id, MTGCardInstance * _source, Targetable * _target, WParsedInt * life, ManaCost * _cost, int doTap,
        int who) :
    ActivatedAbilityTP(_id, _source, _target, _cost, doTap, who), life(life)
{
}

int AALifeSet::resolve()
{
    Damageable * _target = (Damageable *) getTarget();
    if (!_target)
        return 0;

    Player * p = NULL;
    if (_target->type_as_damageable == DAMAGEABLE_MTGCARDINSTANCE)
    {
        p = ((MTGCardInstance *) _target)->controller();
    }
    else
    {
        p = (Player*)_target;
    }

    int lifeDiff = life->getValue() - p->life ;
    p->gainOrLoseLife(lifeDiff);

    return 1;
}

const char * AALifeSet::getMenuText()
{
    return "Set Life";
}

AALifeSet * AALifeSet::clone() const
{
    AALifeSet * a = NEW AALifeSet(*this);
    a->life = NEW WParsedInt(*(a->life));
    a->isClone = 1;
    return a;
}

AALifeSet::~AALifeSet()
{
    SAFE_DELETE(life);
}

//AACloner 
//cloning...this makes a token thats a copy of the target.
AACloner::AACloner(int _id, MTGCardInstance * _source, MTGCardInstance * _target, ManaCost * _cost, int who,
        string abilitiesStringList) :
    ActivatedAbility(_id, _source, _cost, 0, 0), who(who)
{
    aType = MTGAbility::CLONING;
    target = _target;
    source = _source;
    if (abilitiesStringList.size() > 0)
    {
        PopulateAbilityIndexVector(awith, abilitiesStringList);
        PopulateColorIndexVector(colors, abilitiesStringList);
    }

}

    int AACloner::resolve()
    {
        MTGCardInstance * _target = (MTGCardInstance *) target;
        if (_target)
        {
            MTGCardInstance * myClone = NULL;
            MTGCard* clone = (_target->isToken ? _target: MTGCollection()->getCardByName(_target->name));
            if (who != 1)
            {
                myClone = NEW MTGCardInstance(clone, source->controller()->game);
            }
            if (who == 1)
            {
                myClone = NEW MTGCardInstance(clone, source->controller()->opponent()->game);
            }
            if (who != 1)
                source->controller()->game->temp->addCard(myClone);
            else
                source->controller()->opponent()->game->temp->addCard(myClone);
            Spell * spell = NEW Spell(myClone);
            spell->resolve();
            spell->source->isToken = 1;
            spell->source->fresh = 1;
            if(_target->isToken)
            {
            spell->source->power = _target->origpower;
            spell->source->toughness = _target->origtoughness;
            spell->source->life = _target->origtoughness;
            }
            list<int>::iterator it;
            for (it = awith.begin(); it != awith.end(); it++)
        {
            spell->source->basicAbilities[*it] = 1;
        }
        for (it = colors.begin(); it != colors.end(); it++)
        {
            spell->source->setColor(*it);
        }
        delete spell;
        return 1;
    }
    return 0;
}

const char * AACloner::getMenuText()
{
    if (who == 1)
        return "Clone For Opponent";
    return "Clone";
}

ostream& AACloner::toString(ostream& out) const
{
    out << "AACloner ::: with : ?" // << abilities
            << " (";
    return ActivatedAbility::toString(out) << ")";
}

AACloner * AACloner::clone() const
{
    AACloner * a = NEW AACloner(*this);
    a->isClone = 1;
    return a;
}
AACloner::~AACloner()
{
}

// More Land - allow more lands to be played on a turn
AAMoreLandPlz::AAMoreLandPlz(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, WParsedInt * _additional,
        int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who), additional(_additional)
{
}

int AAMoreLandPlz::resolve()
{
    Targetable * _target = getTarget();
    Player * player;
    if (_target)
    {
        if (_target->typeAsTarget() == TARGET_CARD)
        {
            player = ((MTGCardInstance *) _target)->controller();
        }
        else
        {
            player = (Player *) _target;
        }
        player->landsPlayerCanStillPlay += additional->getValue();
    }
    return 1;
}

const char * AAMoreLandPlz::getMenuText()
{
    return "Additional Lands";
}

AAMoreLandPlz * AAMoreLandPlz::clone() const
{
    AAMoreLandPlz * a = NEW AAMoreLandPlz(*this);
    a->additional = NEW WParsedInt(*(a->additional));
    a->isClone = 1;
    return a;
}

AAMoreLandPlz::~AAMoreLandPlz()
{
    SAFE_DELETE(additional);
}

//AAMover
AAMover::AAMover(int _id, MTGCardInstance * _source, MTGCardInstance * _target, string dest, ManaCost * _cost, int doTap) :
    ActivatedAbility(_id, _source, _cost, 0, doTap), destination(dest)
{
    if (_target)
        target = _target;
}

MTGGameZone * AAMover::destinationZone()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    return MTGGameZone::stringToZone(destination, source, _target);
}

int AAMover::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (target)
    {
        Player* p = _target->controller();
        if (p)
        {
            GameObserver * g = GameObserver::GetInstance();
            MTGGameZone * fromZone = _target->getCurrentZone();
            MTGGameZone * destZone = destinationZone();

            //inplay is a special zone !
            for (int i = 0; i < 2; i++)
            {
                if (destZone == g->players[i]->game->inPlay && fromZone != g->players[i]->game->inPlay && fromZone
                        != g->players[i]->opponent()->game->inPlay)
                {
                    MTGCardInstance * copy = g->players[i]->game->putInZone(_target, fromZone, g->players[i]->game->temp);
                    Spell * spell = NEW Spell(copy);
                    spell->resolve();
                    delete spell;
                    return 1;
                }
            }
            p->game->putInZone(_target, fromZone, destZone);
            return 1;
        }
    }
    return 0;
}

const char * AAMover::getMenuText()
{
    return "Move";
}

AAMover * AAMover::clone() const
{
    AAMover * a = NEW AAMover(*this);
    a->isClone = 1;
    return a;
}

// No Creatures
AANoCreatures::AANoCreatures(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who)
{
}

int AANoCreatures::resolve()
{
    Targetable * _target = getTarget();
    Player * player;
    if (_target)
    {
        if (_target->typeAsTarget() == TARGET_CARD)
        {
            player = ((MTGCardInstance *) _target)->controller();
        }
        else
        {
            player = (Player *) _target;
        }
        player->nocreatureinstant = true;
    }
    return 1;
}

const char * AANoCreatures::getMenuText()
{
    return "No Creatures!";
}

AANoCreatures * AANoCreatures::clone() const
{
    AANoCreatures * a = NEW AANoCreatures(*this);
    a->isClone = 1;
    return a;
}

// AA No Spells
AANoSpells::AANoSpells(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who)
{
}
int AANoSpells::resolve()
{
    Targetable * _target = getTarget();
    Player * player;
    if (_target)
    {
        if (_target->typeAsTarget() == TARGET_CARD)
        {
            player = ((MTGCardInstance *) _target)->controller();
        }
        else
        {
            player = (Player *) _target;
        }
        player->nospellinstant = true;
    }
    return 1;
}

const char * AANoSpells::getMenuText()
{
    return "No Spells!";
}

AANoSpells * AANoSpells::clone() const
{
    AANoSpells * a = NEW AANoSpells(*this);
    a->isClone = 1;
    return a;
}

//OnlyOne
AAOnlyOne::AAOnlyOne(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who)
{
}

int AAOnlyOne::resolve()
{
    Targetable * _target = getTarget();
    Player * player;
    if (_target)
    {
        if (_target->typeAsTarget() == TARGET_CARD)
        {
            player = ((MTGCardInstance *) _target)->controller();
        }
        else
        {
            player = (Player *) _target;
        }
        player->onlyoneinstant = true;
    }
    return 1;
}

const char * AAOnlyOne::getMenuText()
{
    return "Only One Spell!";
}

AAOnlyOne * AAOnlyOne::clone() const
{
    AAOnlyOne * a = NEW AAOnlyOne(*this);
    a->isClone = 1;
    return a;
}

//Random Discard
AARandomDiscarder::AARandomDiscarder(int _id, MTGCardInstance * card, Targetable * _target,string nbcardsStr, ManaCost * _cost,
        int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who), nbcardsStr(nbcardsStr)
{
}

int AARandomDiscarder::resolve()
{
    Targetable * _target = getTarget();
    Player * player;
    if (_target)
    {
        if (_target->typeAsTarget() == TARGET_CARD)
        {
            player = ((MTGCardInstance *) _target)->controller();
        }
        else
        {
            player = (Player *) _target;
        }


        WParsedInt numCards(nbcardsStr, NULL, source);
        for (int i = 0; i < numCards.intValue; i++)
        {
            player->game->discardRandom(player->game->hand, source);
        }
    }
            
    return 1;
}

const char * AARandomDiscarder::getMenuText()
{
    return "Discard Random";
}

AARandomDiscarder * AARandomDiscarder::clone() const
{
    AARandomDiscarder * a = NEW AARandomDiscarder(*this);
    a->isClone = 1;
    return a;
}

// Shuffle 
AAShuffle::AAShuffle(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who)
{
}

int AAShuffle::resolve()
{
    Targetable * _target = getTarget();
    Player * player;
    if (_target)
    {
        if (_target->typeAsTarget() == TARGET_CARD)
        {
            player = ((MTGCardInstance *) _target)->controller();
        }
        else
        {
            player = (Player *) _target;
        }
        MTGLibrary * library = player->game->library;
        library->shuffle();
    }
    return 1;
}

const char * AAShuffle::getMenuText()
{
    return "Shuffle";
}

AAShuffle * AAShuffle::clone() const
{
    AAShuffle * a = NEW AAShuffle(*this);
    a->isClone = 1;
    return a;
}

//Tapper
AATapper::AATapper(int id, MTGCardInstance * card, MTGCardInstance * _target, ManaCost * _cost, int doTap) :
    ActivatedAbility(id, card, _cost, 0, doTap)
{
    target = _target;
    aType = MTGAbility::TAPPER;
}

int AATapper::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next; //This is for cards such as rampant growth
        _target->tap();
    }
    return 1;
}

const char * AATapper::getMenuText()
{
    return "Tap";
}

AATapper * AATapper::clone() const
{
    AATapper * a = NEW AATapper(*this);
    a->isClone = 1;
    return a;
}

//AA Untapper
AAUntapper::AAUntapper(int id, MTGCardInstance * card, MTGCardInstance * _target, ManaCost * _cost, int doTap) :
    ActivatedAbility(id, card, _cost, 0, doTap)
{
    target = _target;
    aType = MTGAbility::UNTAPPER;
}

int AAUntapper::resolve()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next; //This is for cards such as rampant growth
        _target->untap();
    }
    return 1;
}

const char * AAUntapper::getMenuText()
{
    return "Untap";
}

AAUntapper * AAUntapper::clone() const
{
    AAUntapper * a = NEW AAUntapper(*this);
    a->isClone = 1;
    return a;
}

AAWhatsMax::AAWhatsMax(int id, MTGCardInstance * card, MTGCardInstance * source, ManaCost * _cost, int doTap, int value) :
    ActivatedAbility(id, card, _cost, 0, doTap), value(value)
{
}

int AAWhatsMax::resolve()
{

    if (source)
    {
        source->MaxLevelUp = value;
        source->isLeveler = 1;
    }
    return 1;
}

AAWhatsMax * AAWhatsMax::clone() const
{
    AAWhatsMax * a = NEW AAWhatsMax(*this);
    a->isClone = 1;
    return a;
}

// Win Game
AAWinGame::AAWinGame(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, int _tap, int who) :
    ActivatedAbilityTP(_id, card, _target, _cost, _tap, who)
{
}

int AAWinGame::resolve()
{
    Damageable * _target = (Damageable *) getTarget();
    if (_target)
    {
        if (_target->type_as_damageable == DAMAGEABLE_MTGCARDINSTANCE)
        {
            _target = ((MTGCardInstance *) _target)->controller();
        }
        int cantlosers = 0;
        MTGGameZone * z = ((Player *) _target)->opponent()->game->inPlay;
        int nbcards = z->nb_cards;

        for (int i = 0; i < nbcards; i++)
        {
            MTGCardInstance * c = z->cards[i];
            if (c->has(Constants::CANTLOSE))
            {
                cantlosers++;
            }
        }
        MTGGameZone * k = ((Player *) _target)->game->inPlay;
        int onbcards = k->nb_cards;
        for (int m = 0; m < onbcards; ++m)
        {
            MTGCardInstance * e = k->cards[m];
            if (e->has(Constants::CANTWIN))
            {
                cantlosers++;
            }
        }
        if (cantlosers < 1)
        {
            game->gameOver = ((Player *) _target)->opponent();
        }
    }
    return 1;
}

const char * AAWinGame::getMenuText()
{
    return "Win Game";
}

AAWinGame * AAWinGame::clone() const
{
    AAWinGame * a = NEW AAWinGame(*this);
    a->isClone = 1;
    return a;
}

//Generic Abilities

//May Abilities
MayAbility::MayAbility(int _id, MTGAbility * _ability, MTGCardInstance * _source, bool must) :
    MTGAbility(_id, _source), NestedAbility(_ability), must(must)
{
    triggered = 0;
    mClone = NULL;
}

void MayAbility::Update(float dt)
{
    MTGAbility::Update(dt);
    if (!triggered)
    {
        triggered = 1;
        if (TargetAbility * ta = dynamic_cast<TargetAbility *>(ability))
        {
            if (!ta->tc->validTargetsExist())
                return;
        }
        game->mLayers->actionLayer()->setMenuObject(source, must);
        game->mLayers->stackLayer()->setIsInterrupting(source->controller());
    }
}

const char * MayAbility::getMenuText()
{
    return ability->getMenuText();
}

int MayAbility::testDestroy()
{
    if (!triggered)
        return 0;
    if (game->mLayers->actionLayer()->menuObject)
        return 0;
    if (game->mLayers->actionLayer()->getIndexOf(mClone) != -1)
        return 0;
    return 1;
}

int MayAbility::isReactingToTargetClick(Targetable * card)
{
    if (card == source)
        return 1;
    return 0;
}

int MayAbility::reactToTargetClick(Targetable * object)
{
    mClone = ability->clone();
    mClone->addToGame();
    mClone->forceDestroy = 1;
    return mClone->reactToTargetClick(object);
}

MayAbility * MayAbility::clone() const
{
    MayAbility * a = NEW MayAbility(*this);
    a->ability = ability->clone();
    a->isClone = 1;
    return a;
}

MayAbility::~MayAbility()
{
    SAFE_DELETE(ability);
}

//MultiAbility : triggers several actions for a cost
MultiAbility::MultiAbility(int _id, MTGCardInstance * card, Targetable * _target, ManaCost * _cost, int _tap) :
    ActivatedAbility(_id, card, _cost, 0, _tap)
{
    if (_target)
        target = _target;
}

int MultiAbility::Add(MTGAbility * ability)
{
    abilities.push_back(ability);
    return 1;
}

int MultiAbility::resolve()
{
    Targetable * Phaseactiontarget = NULL;
    vector<int>::size_type sz = abilities.size();
    for (unsigned int i = 0; i < sz; i++)
    {
        if (abilities[i] == NULL)
            continue;
        Targetable * backup = abilities[i]->target;


        if (target && target != source && abilities[i]->target == abilities[i]->source)
        {
            abilities[i]->target = target;
            Phaseactiontarget = target;
        }
        abilities[i]->resolve();
        abilities[i]->target = backup;
        if(dynamic_cast<APhaseActionGeneric *> (abilities[i]))
        {
            if(Phaseactiontarget != NULL)
                dynamic_cast<APhaseActionGeneric *> (abilities[i])->target = Phaseactiontarget;
        }

    }
    return 1;
}

const char * MultiAbility::getMenuText()
{
    if (abilities.size())
        return abilities[0]->getMenuText();
    return "";
}

MultiAbility * MultiAbility::clone() const
{
    MultiAbility * a = NEW MultiAbility(*this);
    a->isClone = 1;
    return a;
}

MultiAbility::~MultiAbility()
{
    if (!isClone)
    {
        vector<int>::size_type sz = abilities.size();
        for (size_t i = 0; i < sz; i++)
        {
            SAFE_DELETE(abilities[i]);
        }
    }
    abilities.clear();
}

//Generic Target Ability
GenericTargetAbility::GenericTargetAbility(int _id, MTGCardInstance * _source, TargetChooser * _tc, MTGAbility * a,
        ManaCost * _cost, int _tap, int limit, int restrictions, MTGGameZone * dest) :
    TargetAbility(_id, _source, _tc, _cost, restrictions, _tap), limitPerTurn(limit), activeZone(dest)
{
    ability = a;
    MTGAbility * core = AbilityFactory::getCoreAbility(a);
    if (dynamic_cast<AACopier *> (core))
        tc->other = true; //http://code.google.com/p/wagic/issues/detail?id=209 (avoid inifinite loop)
    counters = 0;
}

const char * GenericTargetAbility::getMenuText()
{
    if (!ability)
        return "Error";

    MTGAbility * core = AbilityFactory::getCoreAbility(ability);
    if (AAMover * move = dynamic_cast<AAMover *>(core))
    {
        MTGGameZone * dest = move->destinationZone();
        GameObserver * g = GameObserver::GetInstance();
        for (int i = 0; i < 2; i++)
        {
            if (dest == g->players[i]->game->hand && tc->targetsZone(g->players[i]->game->inPlay))
            {
                return "Bounce";
            }
            else if (dest == g->players[i]->game->hand && tc->targetsZone(g->players[i]->game->graveyard))
            {
                return "Reclaim";
            }
            else if (dest == g->players[i]->game->graveyard && tc->targetsZone(g->players[i]->game->inPlay))
            {
                return "Sacrifice";
            }
            else if (dest == g->players[i]->game->library && tc->targetsZone(g->players[i]->game->graveyard))
            {
                return "Recycle";
            }
            else if (dest == g->players[i]->game->battlefield && tc->targetsZone(g->players[i]->game->graveyard))
            {
                return "Reanimate";
            }
            else if ((tc->targetsZone(g->players[i]->game->inPlay)
                    && dest == g->players[i]->game->library)
                    || dest == g->players[i]->game->library)
            {
                return "Put in Library";
            }
            else if (dest == g->players[i]->game->inPlay)
            {
                return "Put in Play";
            }
            else if (dest == g->players[i]->game->graveyard && tc->targetsZone(g->players[i]->game->hand))
            {
                return "Discard";
            }
            else if (dest == g->players[i]->game->exile)
            {
                return "Exile";
            }
            else if (tc->targetsZone(g->players[i]->game->library))
            {
                return "Fetch";
            }
            else if (dest == g->players[i]->game->hand && tc->targetsZone(g->opponent()->game->hand))
            {
                return "Steal";
            }
            else if (dest == g->players[i]->game->graveyard && tc->targetsZone(g->opponent()->game->hand))
            {
                return "Opponent Discards";
            }
        }
    }

    return ability->getMenuText();

}

int GenericTargetAbility::resolve()
{
    counters++;
    return TargetAbility::resolve();
}

int GenericTargetAbility::isReactingToClick(MTGCardInstance * card, ManaCost * mana)
{
    if (limitPerTurn && counters >= limitPerTurn)
        return 0;
    return TargetAbility::isReactingToClick(card, mana);
}

void GenericTargetAbility::Update(float dt)
{
    if (newPhase != currentPhase && newPhase == Constants::MTG_PHASE_AFTER_EOT)
    {
        counters = 0;
    }
    TargetAbility::Update(dt);
}

int GenericTargetAbility::testDestroy()
{
    if (!activeZone)
        return TargetAbility::testDestroy();
    if (activeZone->hasCard(source))
        return 0;
    return 1;

}

GenericTargetAbility * GenericTargetAbility::clone() const
{
    GenericTargetAbility * a = NEW GenericTargetAbility(*this);
    a->ability = ability->clone();
    a->cost = NEW ManaCost();
    a->cost->copy(cost);
    if (tc)
        a->tc = tc->clone();
    return a;
}

GenericTargetAbility::~GenericTargetAbility()
{
    SAFE_DELETE(ability);
}

//Alter Cost
AAlterCost::AAlterCost(int id, MTGCardInstance * source, MTGCardInstance * target, int amount, int type) :
    MTGAbility(id, source, target), amount(amount), type(type)
{
}

int AAlterCost::addToGame()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (amount < 0)
    {
        amount = abs(amount);
        if (_target->getManaCost()->hasColor(type))
        {
            if (_target->getManaCost()->getConvertedCost() >= 1)
            {
                _target->getManaCost()->remove(type, amount);
                if (_target->getManaCost()->alternative > 0)
                {
                    _target->getManaCost()->alternative->remove(type, amount);
                }
                if (_target->getManaCost()->BuyBack > 0)
                {
                    _target->getManaCost()->BuyBack->remove(type, amount);
                }
            }
        }
    }
    else
    {
        _target->getManaCost()->add(type, amount);
        if (_target->getManaCost()->alternative > 0)
        {
            _target->getManaCost()->alternative->add(type, amount);
        }
        if (_target->getManaCost()->BuyBack > 0)
        {
            _target->getManaCost()->BuyBack->add(type, amount);
        }
    }
    return MTGAbility::addToGame();
}

AAlterCost * AAlterCost::clone() const
{
    AAlterCost * a = NEW AAlterCost(*this);
    a->isClone = 1;
    return a;
}

AAlterCost::~AAlterCost()
{
}

// ATransformer
ATransformer::ATransformer(int id, MTGCardInstance * source, MTGCardInstance * target, string stypes, string sabilities,int newpower,bool newpowerfound,int newtoughness,bool newtoughnessfound) :
    MTGAbility(id, source, target),newpower(newpower),newpowerfound(newpowerfound),newtoughness(newtoughness),newtoughnessfound(newtoughnessfound)
{

    PopulateAbilityIndexVector(abilities, sabilities);
    PopulateColorIndexVector(colors, sabilities);

    remove = false;
    if (stypes == "removesubtypes")
        remove = true;
    if (stypes == "allsubtypes" || stypes == "removesubtypes")
    {
        for (int i = Subtypes::LAST_TYPE + 1;; i++)
        {
            string s = Subtypes::subtypesList->find(i);
            {
                if (s == "")
                    break;
                if (s.find(" ") != string::npos)
                    continue;
                if (s == "Nothing" || s == "Swamp" || s == "Plains" || s == "Mountain" || s == "Forest"
                        || s == "Island" || s == "Shrine" || s == "Basic" || s == "Colony" || s == "Desert"
                        || s == "Dismiss" || s == "Equipment" || s == "Everglades" || s == "Grasslands" || s == "Lair"
                        || s == "Level" || s == "Levelup" || s == "Mine" || s == "Oasis" || s == "World" || s == "Aura"
                )
                {//dont add "nothing" or land type to this card.
                }
                else
                {
                    types.push_back(i);
                }
            }
        }
    }
    else
    {
        PopulateSubtypesIndexVector(types, stypes);
    }

    menu = stypes;
}

int ATransformer::addToGame()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next;
        for (int j = 0; j < Constants::MTG_NB_COLORS; j++)
        {
            if (_target->hasColor(j))
                oldcolors.push_back(j);
        }
        for (int j = Subtypes::LAST_TYPE + 1;; j++)
        {
            string otypes = Subtypes::subtypesList->find(j);
            if (otypes == "")
                break;
            if (otypes.find(" ") != string::npos)
                continue;
            if (_target->hasSubtype(j))
            {
                oldtypes.push_back(j);
            }
        }
        list<int>::iterator it;
        for (it = colors.begin(); it != colors.end(); it++)
        {
            _target->setColor(0, 1);
        }

        for (it = types.begin(); it != types.end(); it++)
        {
            if (remove )
            {
                _target->removeType(*it);
            }
            else
            {
                _target->addType(*it);
            }
        }
        for (it = colors.begin(); it != colors.end(); it++)
        {
            _target->setColor(*it);
        }
        for (it = abilities.begin(); it != abilities.end(); it++)
        {
            _target->basicAbilities[*it]++;
        }
        for (it = oldcolors.begin(); it != oldcolors.end(); it++)
        {
        }
        if(newpowerfound )
        {
            oldpower = _target->power;
            _target->power += newpower;
            _target->power -= oldpower;
        }
        if(newtoughnessfound )
        {
            oldtoughness = _target->toughness;
            _target->addToToughness(newtoughness);
            _target->addToToughness(-oldtoughness);
        }
    }
    return MTGAbility::addToGame();
}

int ATransformer::destroy()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next;
        list<int>::iterator it;
        for (it = types.begin(); it != types.end(); it++)
        {
            if (remove == false)
                _target->removeType(*it);
        }
        for (it = colors.begin(); it != colors.end(); it++)
        {
            _target->removeColor(*it);
        }
        for (it = abilities.begin(); it != abilities.end(); it++)
        {
            _target->basicAbilities[*it]--;
        }
        for (it = oldcolors.begin(); it != oldcolors.end(); it++)
        {
            _target->setColor(*it);
        }
        if (remove )
        {
            for (it = oldtypes.begin(); it != oldtypes.end(); it++)
            {
                if (!_target->hasSubtype(*it))
                    _target->addType(*it);
            }
        }
        if(newpowerfound )
        {
            _target->power = oldpower;
        }
        if(newtoughnessfound )
        {
            _target->toughness = oldtoughness;
        }
    }
    return 1;
}

const char * ATransformer::getMenuText()
{
    string s = menu;
    sprintf(menuText, "Becomes %s", s.c_str());
    return menuText;
}

ATransformer * ATransformer::clone() const
{
    ATransformer * a = NEW ATransformer(*this);
    a->isClone = 1;
    return a;
}

ATransformer::~ATransformer()
{
}

// AForeverTransformer
AForeverTransformer::AForeverTransformer(int id, MTGCardInstance * source, MTGCardInstance * target, string stypes,
        string sabilities,int newpower,bool newpowerfound, int newtoughness,bool newtoughnessfound) :
    MTGAbility(id, source, target),newpower(newpower),newpowerfound(newpowerfound),newtoughness(newtoughness),newtoughnessfound(newtoughnessfound)
{
    aType = MTGAbility::STANDARD_BECOMES;

    PopulateAbilityIndexVector(abilities, sabilities);
    PopulateColorIndexVector(colors, sabilities);
    PopulateSubtypesIndexVector(types, stypes);
    menu = stypes;
    
    remove = false;
    if (stypes == "removetypes")
        remove = true;
}

int AForeverTransformer::addToGame()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        while (_target->next)
            _target = _target->next;
        list<int>::iterator it;
        for (it = colors.begin(); it != colors.end(); it++)
        {
            _target->setColor(0, 1);
        }
        for (it = types.begin(); it != types.end(); it++)
        {
            if(remove)
            {
            _target->removeType(0,1);
            _target->removeType(1,1);
            _target->removeType(2,1);
            _target->removeType(3,1);
            _target->removeType(4,1);
            _target->removeType(5,1);
            _target->removeType(6,1);
            _target->removeType(7,1);
            }
            else
                _target->addType(*it);
        }
        for (it = colors.begin(); it != colors.end(); it++)
        {
            _target->setColor(*it);
        }
        for (it = abilities.begin(); it != abilities.end(); it++)
        {
            _target->basicAbilities[*it]++;
        }
        if(newpowerfound )
        {
            oldpower = _target->power -= oldpower;
            _target->power += newpower;
            _target->power -= oldpower;
        }
        if(newtoughnessfound )
        {
            oldtoughness = _target->toughness;
            _target->addToToughness(newtoughness);
            _target->addToToughness(-oldtoughness);
        }
    }
    return MTGAbility::addToGame();
}

const char * AForeverTransformer::getMenuText()
{
    string s = menu;
    sprintf(menuText, "Becomes %s", s.c_str());
    return menuText;
}

AForeverTransformer * AForeverTransformer::clone() const
{
    AForeverTransformer * a = NEW AForeverTransformer(*this);
    a->isClone = 1;
    return a;
}
AForeverTransformer::~AForeverTransformer()
{
}

//ATransformerUEOT
ATransformerUEOT::ATransformerUEOT(int id, MTGCardInstance * source, MTGCardInstance * target, string types, string abilities,int newpower,bool newpowerfound,int newtoughness,bool newtoughnessfound) :
    InstantAbility(id, source, target),newpower(newpower),newpowerfound(newpowerfound),newtoughness(newtoughness),newtoughnessfound(newtoughnessfound)
{
    ability = NEW ATransformer(id, source, target, types, abilities,newpower,newpowerfound,newtoughness,newtoughnessfound);
    aType = MTGAbility::STANDARD_BECOMES;
}

int ATransformerUEOT::resolve()
{
    ATransformer * a = ability->clone();
    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source, (Damageable *) (this->target), a);
    wrapper->addToGame();
    return 1;
}
const char * ATransformerUEOT::getMenuText()
{
    return ability->getMenuText();
}

ATransformerUEOT * ATransformerUEOT::clone() const
{
    ATransformerUEOT * a = NEW ATransformerUEOT(*this);
    a->ability = this->ability->clone();
    a->isClone = 1;
    return a;
}

ATransformerUEOT::~ATransformerUEOT()
{
    SAFE_DELETE(ability);
}

// ATransformerFOREVER
ATransformerFOREVER::ATransformerFOREVER(int id, MTGCardInstance * source, MTGCardInstance * target, string types, string abilities,int newpower,bool newpowerfound,int newtoughness,bool newtoughnessfound) :
    InstantAbility(id, source, target),newpower(newpower),newpowerfound(newpowerfound),newtoughness(newtoughness),newtoughnessfound(newtoughnessfound)
{
    ability = NEW AForeverTransformer(id, source, target, types, abilities,newpower,newpowerfound,newtoughness,newtoughnessfound);
    aType = MTGAbility::STANDARD_BECOMES;
}

int ATransformerFOREVER::resolve()
{
    AForeverTransformer * a = ability->clone();
    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source, (Damageable *) (this->target), a);
    wrapper->addToGame();
    return 1;
}

const char * ATransformerFOREVER::getMenuText()
{
    return ability->getMenuText();
}

ATransformerFOREVER * ATransformerFOREVER::clone() const
{
    ATransformerFOREVER * a = NEW ATransformerFOREVER(*this);
    a->ability = this->ability->clone();
    a->isClone = 1;
    return a;
}

ATransformerFOREVER::~ATransformerFOREVER()
{
    SAFE_DELETE(ability);
}

// ASwapPTUEOT
ASwapPTUEOT::ASwapPTUEOT(int id, MTGCardInstance * source, MTGCardInstance * target) :
    InstantAbility(id, source, target)
{
    ability = NEW ASwapPT(id, source, target);
}

int ASwapPTUEOT::resolve()
{
    ASwapPT * a = ability->clone();
    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source, (Damageable *) (this->target), a);
    wrapper->addToGame();
    return 1;
}

const char * ASwapPTUEOT::getMenuText()
{
    return ability->getMenuText();
}

ASwapPTUEOT * ASwapPTUEOT::clone() const
{
    ASwapPTUEOT * a = NEW ASwapPTUEOT(*this);
    a->ability = this->ability->clone();
    a->isClone = 1;
    return a;
}

ASwapPTUEOT::~ASwapPTUEOT()
{
    SAFE_DELETE(ability);
}

// ABecomes
ABecomes::ABecomes(int id, MTGCardInstance * source, MTGCardInstance * target, string stypes, WParsedPT * wppt, string sabilities) :
    MTGAbility(id, source, target), wppt(wppt)
{

    aType = MTGAbility::STANDARD_BECOMES;

    PopulateAbilityIndexVector(abilities, sabilities);
    PopulateColorIndexVector(colors, sabilities);
    PopulateSubtypesIndexVector(types, stypes);
    menu = stypes;

}
int ABecomes::addToGame()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    list<int>::iterator it;
    for (it = types.begin(); it != types.end(); it++)
    {
        _target->addType(*it);
    }
    for (it = colors.begin(); it != colors.end(); it++)
    {
        _target->setColor(*it);
    }
    for (it = abilities.begin(); it != abilities.end(); it++)
    {
        _target->basicAbilities[*it]++;
    }

    if (wppt)
    {
        _target->power = wppt->power.getValue();
        _target->toughness = wppt->toughness.getValue();
        _target->life = _target->toughness;
    }
    return MTGAbility::addToGame();
}

int ABecomes::destroy()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    list<int>::iterator it;
    for (it = types.begin(); it != types.end(); it++)
    {
        _target->removeType(*it);
    }
    for (it = colors.begin(); it != colors.end(); it++)
    {
        _target->removeColor(*it);
    }
    for (it = abilities.begin(); it != abilities.end(); it++)
    {
        _target->basicAbilities[*it]--;
    }
    return 1;
}

const char * ABecomes::getMenuText()
{
    string s = menu;
    sprintf(menuText, "Becomes %s", s.c_str());
    return menuText;
}

ABecomes * ABecomes::clone() const
{
    ABecomes * a = NEW ABecomes(*this);
    if (a->wppt)
        a->wppt = NEW WParsedPT(*(a->wppt));
    a->isClone = 1;
    return a;
}

ABecomes::~ABecomes()
{
    SAFE_DELETE (wppt);
}

//  ABecomes

// ABecomesUEOT
ABecomesUEOT::ABecomesUEOT(int id, MTGCardInstance * source, MTGCardInstance * target, string types, WParsedPT * wpt,
        string abilities) :
    InstantAbility(id, source, target)
{
    ability = NEW ABecomes(id, source, target, types, wpt, abilities);
    aType = MTGAbility::STANDARD_BECOMES;
}

int ABecomesUEOT::resolve()
{
    ABecomes * a = ability->clone();
    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source, (Damageable *) (this->target), a);
    wrapper->addToGame();
    return 1;
}

const char * ABecomesUEOT::getMenuText()
{
    return ability->getMenuText();
}

ABecomesUEOT * ABecomesUEOT::clone() const
{
    ABecomesUEOT * a = NEW ABecomesUEOT(*this);
    a->ability = this->ability->clone();
    a->isClone = 1;
    return a;
}

ABecomesUEOT::~ABecomesUEOT()
{
    SAFE_DELETE(ability);
}

//APreventDamageTypes
APreventDamageTypes::APreventDamageTypes(int id, MTGCardInstance * source, string to, string from, int type) :
    MTGAbility(id, source), to(to), from(from), type(type)
{
    re = NULL;
}

int APreventDamageTypes::addToGame()
{
    if (re)
    {
        DebugTrace("FATAL:re shouldn't be already set in APreventDamageTypes\n");
        return 0;
    }
    TargetChooserFactory tcf;
    TargetChooser *toTc = tcf.createTargetChooser(to, source, this);
    if (toTc)
        toTc->targetter = NULL;
    TargetChooser *fromTc = tcf.createTargetChooser(from, source, this);
    if (fromTc)
        fromTc->targetter = NULL;
    if (type != 1 && type != 2)
    {//not adding this creates a memory leak.
        re = NEW REDamagePrevention(this, fromTc, toTc, -1, false, DAMAGE_COMBAT);
    }
    else if (type == 1)
    {
        re = NEW REDamagePrevention(this, fromTc, toTc, -1, false, DAMAGE_ALL_TYPES);
    }
    else if (type == 2)
    {
        re = NEW REDamagePrevention(this, fromTc, toTc, -1, false, DAMAGE_OTHER);
    }
    game->replacementEffects->add(re);
    return MTGAbility::addToGame();
}

int APreventDamageTypes::destroy()
{
    game->replacementEffects->remove(re);
    SAFE_DELETE(re);
    return 1;
}

APreventDamageTypes * APreventDamageTypes::clone() const
{
    APreventDamageTypes * a = NEW APreventDamageTypes(*this);
    a->isClone = 1;
    return a;
}

APreventDamageTypes::~APreventDamageTypes()
{
    SAFE_DELETE(re);
}

//APreventDamageTypesUEOT
APreventDamageTypesUEOT::APreventDamageTypesUEOT(int id, MTGCardInstance * source, string to, string from, int type) :
    InstantAbility(id, source)
{
    ability = NEW APreventDamageTypes(id, source, to, from, type);
}

int APreventDamageTypesUEOT::resolve()
{
    APreventDamageTypes * a = ability->clone();
    GenericInstantAbility * wrapper = NEW GenericInstantAbility(1, source, (Damageable *) (this->target), a);
    wrapper->addToGame();
    return 1;
}

int APreventDamageTypesUEOT::destroy()
{
    for (size_t i = 0; i < clones.size(); ++i)
    {
        clones[i]->forceDestroy = 0;
    }
    clones.clear();
    return 1;
}

const char * APreventDamageTypesUEOT::getMenuText()
{
    return ability->getMenuText();
}

APreventDamageTypesUEOT * APreventDamageTypesUEOT::clone() const
{
    APreventDamageTypesUEOT * a = NEW APreventDamageTypesUEOT(*this);
    a->ability = this->ability->clone();
    a->isClone = 1;
    return a;
}

APreventDamageTypesUEOT::~APreventDamageTypesUEOT()
{
    SAFE_DELETE(ability);
}

//AVanishing creature also fading
AVanishing::AVanishing(int _id, MTGCardInstance * card, ManaCost * _cost, int _tap, int restrictions, int amount, string counterName) :
ActivatedAbility(_id, card, _cost, restrictions, _tap),amount(amount),counterName(counterName)
{
    for(int i = 0;i< amount;i++)
        source->counters->addCounter(counterName.c_str(),0,0);
}

void AVanishing::Update(float dt)
{
    if (newPhase != currentPhase && source->controller() == game->currentPlayer)
    {
        if(newPhase == Constants::MTG_PHASE_UPKEEP)
        {
            source->counters->removeCounter(counterName.c_str(),0,0);
            Counter * targetCounter = NULL;
            timeLeft = 0;

            if (source->counters && source->counters->hasCounter(counterName.c_str(), 0, 0))
            {
                targetCounter = source->counters->hasCounter(counterName.c_str(), 0, 0);
                timeLeft = targetCounter->nb;
            }
            else
            {
                timeLeft = 0;
                WEvent * e = NEW WEventCardSacrifice(source);
                GameObserver * game = GameObserver::GetInstance();
                game->receiveEvent(e);
                source->controller()->game->putInGraveyard(source);
            }

        }
        else if (newPhase == Constants::MTG_PHASE_UPKEEP && timeLeft <= 0)
        {
            WEvent * e = NEW WEventCardSacrifice(source);
            GameObserver * game = GameObserver::GetInstance();
            game->receiveEvent(e);
            source->controller()->game->putInGraveyard(source);
        }

    }
    ActivatedAbility::Update(dt);
}

int AVanishing::resolve()
{

    return 1;
}

const char * AVanishing::getMenuText()
{
if(counterName.find("fading") != string::npos)
return "Fading";
    return "Vanishing";
}

AVanishing * AVanishing::clone() const
{
    AVanishing * a = NEW AVanishing(*this);
    a->isClone = 1;
    return a;
}

AVanishing::~AVanishing()
{
}

//AUpkeep
AUpkeep::AUpkeep(int _id, MTGCardInstance * card, MTGAbility * a, ManaCost * _cost, int _tap, int restrictions, int _phase,
        int _once,bool Cumulative) :
    ActivatedAbility(_id, card, _cost, restrictions, _tap), NestedAbility(a), phase(_phase), once(_once),Cumulative(Cumulative)
{
    paidThisTurn = 0;
    aType = MTGAbility::UPCOST;
}

void AUpkeep::Update(float dt)
{
    // once: 0 means always go off, 1 means go off only once, 2 means go off only once and already has.
    if (newPhase != currentPhase && source->controller() == game->currentPlayer && once < 2)
    {
        if (newPhase == Constants::MTG_PHASE_UNTAP)
        {
            paidThisTurn = 0;
        }
        else if(newPhase == Constants::MTG_PHASE_UPKEEP && Cumulative )
        {
            source->counters->addCounter("age",0,0);
                Counter * targetCounter = NULL;
                currentage = 0;

                if (source->counters && source->counters->hasCounter("age", 0, 0))
                {
                    targetCounter = source->counters->hasCounter("age", 0, 0);
                    currentage = targetCounter->nb - 1;
                }
            if(currentage)
                paidThisTurn -= currentage;
        }
        else if (newPhase == phase + 1 && paidThisTurn < 1)
        {
            ability->resolve();
        }
        if (newPhase == phase + 1 && once)
            once = 2;
    }
    ActivatedAbility::Update(dt);
}

int AUpkeep::isReactingToClick(MTGCardInstance * card, ManaCost * mana)
{
    if (currentPhase != phase || paidThisTurn > 0 || once >= 2)
        return 0;
    return ActivatedAbility::isReactingToClick(card, mana);
}

int AUpkeep::resolve()
{
    paidThisTurn += 1;
    return 1;
}

const char * AUpkeep::getMenuText()
{
    return "Upkeep";
}

ostream& AUpkeep::toString(ostream& out) const
{
    out << "AUpkeep ::: paidThisTurn : " << paidThisTurn << " (";
    return ActivatedAbility::toString(out) << ")";
}

AUpkeep * AUpkeep::clone() const
{
    AUpkeep * a = NEW AUpkeep(*this);
    a->isClone = 1;
    return a;
}

AUpkeep::~AUpkeep()
{
    if (!isClone)
        SAFE_DELETE(ability);
}

//A Phase based Action
APhaseAction::APhaseAction(int _id, MTGCardInstance * card, MTGCardInstance * target, MTGAbility * a, int _tap, int restrictions, int _phase,bool forcedestroy,bool next) :
MTGAbility(_id, card), NestedAbility(a), phase(_phase),forcedestroy(forcedestroy),next(next)
{
    abilityOwner = card->controller();
}

void APhaseAction::Update(float dt)
{
    if (newPhase != currentPhase)
    {
        if(newPhase == phase && next )
        {
            MTGCardInstance * _target = (MTGCardInstance *) target;
            if (_target)
            {
                while (_target->next)
                    _target = _target->next;
            }
            ability->target = _target;
            ability->source = _target;
            ability->oneShot = 1;
            ability->resolve();
            this->forceDestroy = 1;
            removeAbility();
        }
        else if(newPhase == phase && next == false)
            next = true;
    }
    MTGAbility::Update(dt);
}

int APhaseAction::resolve()
{
    return 0;
}

const char * APhaseAction::getMenuText()
{
    return ability->getMenuText();
}

APhaseAction * APhaseAction::clone() const
{
    APhaseAction * a = NEW APhaseAction(*this);
    if(forcedestroy == false)
        a->forceDestroy = -1;// we want this ability to stay alive until it resolves.
    a->isClone = 1;
    return a;
}
int APhaseAction::removeAbility()
{
    if (!isClone)
        SAFE_DELETE(ability);
    return 1;
}

APhaseAction::~APhaseAction()
{
}

// the main ability
APhaseActionGeneric::APhaseActionGeneric(int _id, MTGCardInstance * card, MTGCardInstance * target, MTGAbility * a, int _tap, int restrictions, int _phase,bool forcedestroy,bool next) :
    InstantAbility(_id, source, target)
{
    MTGCardInstance * _target = target;
    ability = NEW APhaseAction(_id, card,_target, a,_tap, restrictions, _phase,forcedestroy,next);
}

int APhaseActionGeneric::resolve()
{
        APhaseAction * a = ability->clone();
        a->target = target;
        a->addToGame();
        return 1;
}

const char * APhaseActionGeneric::getMenuText()
{
    return ability->getMenuText();
}

APhaseActionGeneric * APhaseActionGeneric::clone() const
{
    APhaseActionGeneric * a = NEW APhaseActionGeneric(*this);
    a->ability = this->ability->clone();
    a->oneShot = 1;
    a->isClone = 1;
    return a;
}

APhaseActionGeneric::~APhaseActionGeneric()
{
    SAFE_DELETE(ability);
}

//a blink
ABlink::ABlink(int _id, MTGCardInstance * card, MTGCardInstance * _target,bool blinkueot,bool blinkForSource,bool blinkhand,MTGAbility * stored) :
MTGAbility(_id, card),blinkueot(blinkueot),blinkForSource(blinkForSource),blinkhand(blinkhand),stored(stored)
{
    target = _target;
    Blinked = NULL;
    resolved = false;
}

void ABlink::Update(float dt)
{
    if(resolved == false)
    {
        resolved = true;
        resolveBlink();
    }
    GameObserver * game = game->GetInstance();
    if ((blinkueot && currentPhase == Constants::MTG_PHASE_ENDOFTURN)||(blinkForSource && !source->isInPlay()))
    {
        if(Blinked == NULL)
            MTGAbility::Update(dt);
        MTGCardInstance * _target = Blinked;
        MTGCardInstance * Blinker = NULL;
        if(!blinkhand)
            Blinker = _target->controller()->game->putInZone(_target, _target->currentZone,
            _target->owner->game->battlefield);
        if(blinkhand)
        {
            _target->controller()->game->putInZone(_target, _target->currentZone,
                _target->owner->game->hand);
            return;
        }
        Spell * spell = NEW Spell(Blinker);
        spell->source->counters->init();
        if(spell->source->hasSubtype("aura") && !blinkhand)
        {
            TargetChooserFactory tcf;
            TargetChooser * tc = tcf.createTargetChooser(spell->source->spellTargetType,spell->source);
            if(!tc->validTargetsExist())
            {
                spell->source->owner->game->putInExile(spell->source);
                delete spell;
                delete tc;
                this->forceDestroy = 1;
                return;
            }

            MTGGameZone * inplay = spell->source->owner->game->inPlay;
            spell->source->target = NULL;
            for(int i = WRand()%inplay->nb_cards;;i = WRand()%inplay->nb_cards)
            {
                if(tc->canTarget(inplay->cards[i]) && spell->source->target == NULL)
                {
                    spell->source->target = inplay->cards[i];
                    spell->getNextCardTarget();
                    spell->resolve();

                    delete spell;
                    delete tc;
                    this->forceDestroy = 1;
                    return;
                }
                if(!tc->validTargetsExist())
                return;
            }
        }
        spell->source->power = spell->source->origpower;
        spell->source->toughness = spell->source->origtoughness;
        if(!spell->source->hasSubtype("aura"))
        {
            spell->resolve();
            if(stored)
            {
                MTGAbility * clonedStored = stored->clone();
                clonedStored->target = spell->source;
                if (clonedStored->oneShot)
                {
                    clonedStored->resolve();
                    delete (clonedStored);
                }
                else
                {
                    clonedStored->addToGame();
                }
            }
        }
        delete spell;
        this->forceDestroy = 1;
        Blinker = NULL;
        return;
    }
    MTGAbility::Update(dt);
}

void ABlink::resolveBlink()
{
    MTGCardInstance * _target = (MTGCardInstance *) target;
    if (_target)
    {
        if(blinkhand && !_target->controller()->game->isInZone(_target,_target->controller()->game->hand))
        {
            this->forceDestroy = 1;
            return;
        }
        else if(!blinkhand && !_target->controller()->game->isInZone(_target,_target->controller()->game->battlefield))
        {
            this->forceDestroy = 1;
            return;
        }
        _target->controller()->game->putInZone(_target, _target->currentZone,
            _target->owner->game->exile);
        _target = _target->next;
        Blinked = _target;
        if(!blinkueot && !blinkForSource)
        {
            MTGCardInstance * Blinker = NULL;
            if(!blinkhand)
                Blinker = _target->controller()->game->putInZone(_target, _target->currentZone,
                _target->owner->game->battlefield);
            if(blinkhand)
            {
                _target->controller()->game->putInZone(_target, _target->currentZone,
                    _target->owner->game->hand);
                return;
            }
            Spell * spell = NEW Spell(Blinker);
            spell->source->counters->init();
            if(spell->source->hasSubtype("aura") && !blinkhand)
            {
                TargetChooserFactory tcf;
                TargetChooser * tc = tcf.createTargetChooser(spell->source->spellTargetType,spell->source);
                if(!tc->validTargetsExist())
                {
                    spell->source->owner->game->putInExile(spell->source);
                    delete spell;
                    delete tc;
                    this->forceDestroy = 1;
                    return;
                }

                MTGGameZone * inplay = spell->source->owner->game->inPlay;
                spell->source->target = NULL;
                for(int i = WRand()%inplay->nb_cards;;i = WRand()%inplay->nb_cards)
                {
                    if(tc->canTarget(inplay->cards[i]) && spell->source->target == NULL)
                    {
                        spell->source->target = inplay->cards[i];
                        spell->getNextCardTarget();
                        spell->resolve();
                        delete spell;
                        delete tc;
                        this->forceDestroy = 1;
                        return;
                    }
                }
            }
            spell->source->power = spell->source->origpower;
            spell->source->toughness = spell->source->origtoughness;
            spell->resolve();
            if(stored)
            {
                MTGAbility * clonedStored = stored->clone();
                clonedStored->target = spell->source;
                if (clonedStored->oneShot)
                {
                    clonedStored->resolve();
                    delete (clonedStored);
                }
                else
                {
                    clonedStored->addToGame();
                }
            }
            delete tc;
            delete spell;
            this->forceDestroy = 1;
            if(stored)
            delete(stored);
            Blinked = NULL;
        }
    }
}

int ABlink::resolve()
{
    return 0;
}
const char * ABlink::getMenuText()
{
    return "Blink";
}

ABlink * ABlink::clone() const
{
    ABlink * a = NEW ABlink(*this);
    a->isClone = 1;
    a->forceDestroy = -1;
    return a;
};
ABlink::~ABlink()
{
    if (!isClone)
        SAFE_DELETE(stored);
}

ABlinkGeneric::ABlinkGeneric(int _id, MTGCardInstance * card, MTGCardInstance * _target,bool blinkueot,bool blinkForSource,bool blinkhand,MTGAbility * stored) :
    InstantAbility(_id, source, _target)
{
    ability = NEW ABlink(_id,card,_target,blinkueot,blinkForSource,blinkhand,stored);
}

int ABlinkGeneric::resolve()
{
        ABlink * a = ability->clone();
        a->target = target;
        a->addToGame();
        return 1;
}

const char * ABlinkGeneric::getMenuText()
{
    return "Blink";
}

ABlinkGeneric * ABlinkGeneric::clone() const
{
    ABlinkGeneric * a = NEW ABlinkGeneric(*this);
    a->ability = this->ability->clone();
    a->oneShot = 1;
    a->isClone = 1;
    return a;
}

ABlinkGeneric::~ABlinkGeneric()
{
    SAFE_DELETE(ability);
}

// utility functions

// Given a delimited string of abilities, add the ones to the list that are "Basic"  MTG abilities
void PopulateAbilityIndexVector(list<int>& abilities, const string& abilityStringList, char delimiter)
{
    vector<string> abilitiesList = split(abilityStringList, delimiter);
    for (vector<string>::iterator iter = abilitiesList.begin(); iter != abilitiesList.end(); ++iter)
    {
        int abilityIndex = Constants::GetBasicAbilityIndex(*iter);

        if (abilityIndex != -1)
            abilities.push_back(abilityIndex);
    }
}

void PopulateColorIndexVector(list<int>& colors, const string& colorStringList, char delimiter)
{
    vector<string> abilitiesList = split(colorStringList, delimiter);
    for (vector<string>::iterator iter = abilitiesList.begin(); iter != abilitiesList.end(); ++iter)
    {
        for (int colorIndex = Constants::MTG_COLOR_ARTIFACT; colorIndex < Constants::MTG_NB_COLORS; ++colorIndex)
        {
            // if the text is not a basic ability but contains a valid color add it to the color vector
            if ((Constants::GetBasicAbilityIndex(*iter) == -1)
                    && ((*iter).find(Constants::MTGColorStrings[colorIndex]) != string::npos))
                colors.push_back(colorIndex);
        }
    }
}

void PopulateSubtypesIndexVector(list<int>& types, const string& subTypesStringList, char delimiter)
{
    vector<string> subTypesList = split(subTypesStringList, delimiter);
    for (vector<string>::iterator it = subTypesList.begin(); it != subTypesList.end(); ++it)
    {
        string subtype = *it;
        size_t id = Subtypes::subtypesList->find(subtype);
        if (id != string::npos)
            types.push_back(id);
    }
}
