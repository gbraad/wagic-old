#include <iostream>
#include "../include/PlayGuiObject.h"
#include "../include/CardGui.h"
#include "../include/CardSelector.h"
#include "../include/GuiHand.h"
#include "Closest.cpp"

using std::cout;

struct Left : public Exp { static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
  { return ref->x - test->x > fabs(ref->y - test->y); } };
struct Right : public Exp { static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
  { return test->x - ref->x > fabs(ref->y - test->y); } };
struct Up : public Exp { static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
  { return ref->y - test->y > fabs(ref->x - test->x); } };
struct Down : public Exp { static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
  { return test->y - ref->y > fabs(ref->x - test->x); } };
struct Diff : public Exp { static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
  { return ref != test; } };
struct True : public Exp { static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
  { return true; } };

template<>
CardSelector::ObjectSelector(DuelLayers* duel) : active(NULL), showBig(true), duel(duel), limitor(NULL), bigpos(300, 150, 1.0, 0.0, 220) {}

template<>
void CardSelector::Add(CardSelector::Target* target)
{
  if (NULL == active)
    if (NULL == limitor || limitor->select(active))
      active = target;
  CardView* c = dynamic_cast<CardView*>(target);
  if (c) c->zoom = 1.0;
  c = dynamic_cast<CardView*>(active);
  if (c) c->zoom = 1.4;
  cards.push_back(target);
}
template<>
void CardSelector::Remove(CardSelector::Target* card)
{
  for (vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
      if (card == *it)
	{
	  if (active == *it)
	    {
	      CardView* c = dynamic_cast<CardView*>(active); if (c) c->zoom = 1.0;
	      active = closest<Diff>(cards, limitor, active);
	      c = dynamic_cast<CardView*>(active); if (c) c->zoom = 1.4;
	    }
	  if (active == *it) active = NULL;
	  cards.erase(it);
	  return;
	}
    }
}

template<>
bool CardSelector::CheckUserInput(u32 key)
{
  if (!active)
    {
      for (vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
	if ((NULL == limitor) || (limitor->select(*it)))
	  {
	    active = *it;
	    active->Entering();
	    return true;
	  }
      return true;
    }
  Target* oldactive = active;
  switch (key)
    {
    case PSP_CTRL_CIRCLE:
      GameObserver::GetInstance()->ButtonPressed(active);
      return true;
      break;
    case PSP_CTRL_LEFT:
      active = closest<Left>(cards, limitor, active);
      break;
    case PSP_CTRL_RIGHT:
      active = closest<Right>(cards, limitor, active);
      break;
    case PSP_CTRL_UP:
      active = closest<Up>(cards, limitor, active);
      break;
    case PSP_CTRL_DOWN:
      active = closest<Down>(cards, limitor, active);
      break;
    case PSP_CTRL_LTRIGGER:
      showBig = !showBig;
      return true;
    default:
      return false;
    }
  if (active != oldactive) { oldactive->Leaving(key); active->Entering(); }
  return true;
}

template<>
void CardSelector::Update(float dt)
{
  float boundary = duel->RightBoundary();
  float position = boundary - CardGui::BigWidth / 2;
  if (CardView* c = dynamic_cast<CardView*>(active))
    if ((c->x + CardGui::Width / 2 > position - CardGui::BigWidth / 2) &&
	(c->x - CardGui::Width / 2 < position + CardGui::BigWidth / 2))
      position = CardGui::BigWidth / 2 - 10;
  bigpos.x = position;
  bigpos.Update(dt);
}

template<>
void CardSelector::Render()
{
  if (active)
    {
      active->Render();
      if (CardView* c = dynamic_cast<CardView*>(active))
	if (showBig)
	  c->RenderBig(bigpos);
    }
}

template<>
void CardSelector::Limit(LimitorFunctor<Target>* limitor)
{
  this->limitor = limitor;
  if (limitor && !limitor->select(active))
    {
      active = closest<True>(cards, limitor, active);
      if (limitor && !limitor->select(active)) active = NULL;
    }
}
