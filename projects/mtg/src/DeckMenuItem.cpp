#include "PrecompiledHeader.h"

#include "DeckMenuItem.h"
#include "Translate.h"
#include "WResourceManager.h"

DeckMenuItem::DeckMenuItem(DeckMenu* _parent, int id, int fontId, string text, float x, float y, bool hasFocus, bool autoTranslate, DeckMetaData *deckMetaData): JGuiObject(id), parent(_parent), fontId(fontId), mX(x), mY(y)
{
  if (autoTranslate) 
    mText = _(text);
  else 
    mText = text;
  mHasFocus = hasFocus;

  mScale = 1.0f;
  mTargetScale = 1.0f;

  if (hasFocus)
    Entering();

  meta = deckMetaData;
  if ( meta && meta->getAvatarFilename().size() > 0 )
     this->imageFilename = meta->getAvatarFilename();
  else
    this->imageFilename = "avatar.jpg";
  
}


void DeckMenuItem::RenderWithOffset(float yOffset)
{
  WFont * mFont = resources.GetWFont(fontId);
  mFont->DrawString(mText.c_str(), mX, mY + yOffset, JGETEXT_CENTER);
}

void DeckMenuItem::Render()
{
  RenderWithOffset(0);
}

void DeckMenuItem::Update(float dt)
{
    if (mScale < mTargetScale)
    {
      mScale += 8.0f*dt;
      if (mScale > mTargetScale)
	mScale = mTargetScale;
    }
  else if (mScale > mTargetScale)
    {
      mScale -= 8.0f*dt;
      if (mScale < mTargetScale)
	mScale = mTargetScale;
	}
}


void DeckMenuItem::Entering()
{
  mHasFocus = true;
  parent->selectionTargetY = mY;
}


bool DeckMenuItem::Leaving(JButton key)
{
  mHasFocus = false;
  return true;
}


bool DeckMenuItem::ButtonPressed()
{
  return true;
}

void DeckMenuItem::Relocate(float x, float y)
{
  mX = x;
  mY = y;
}

float DeckMenuItem::GetWidth()
{
  WFont * mFont = resources.GetWFont(fontId);
  mFont->SetScale(1.0);
  return mFont->GetStringWidth(mText.c_str());
}

bool DeckMenuItem::hasFocus()
{
  return mHasFocus;
}

ostream& DeckMenuItem::toString(ostream& out) const
{
  return out << "DeckMenuItem ::: mHasFocus : " << mHasFocus
	     << " ; parent : " << parent
	     << " ; mText : " << mText
	     << " ; mScale : " << mScale
	     << " ; mTargetScale : " << mTargetScale
	     << " ; mX,mY : " << mX << "," << mY;
}


DeckMenuItem::~DeckMenuItem()
{
  meta = NULL;
}