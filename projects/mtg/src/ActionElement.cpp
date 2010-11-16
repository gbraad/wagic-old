#include "PrecompiledHeader.h"

#include "ActionElement.h"
#include "MTGCardInstance.h"
#include "Targetable.h"
#include "TargetChooser.h"

ActionElement::ActionElement(int id) :
    JGuiObject(id)
{
    activeState = INACTIVE;
    modal = 0;
    waitingForAnswer = 0;
    currentPhase = -1;
    newPhase = -1;
    tc = NULL;
    isClone = 0;
}

ActionElement::~ActionElement()
{
    if (!isClone)
    {
        SAFE_DELETE(tc);
    }
}

int ActionElement::getActivity()
{

    return activeState;
}

int ActionElement::isReactingToTargetClick(Targetable * object)
{
    if (object && object->typeAsTarget() == TARGET_CARD)
        return isReactingToClick((MTGCardInstance *) object);
    return 0;
}

int ActionElement::reactToTargetClick(Targetable * object)
{
    if (object->typeAsTarget() == TARGET_CARD)
        return reactToClick((MTGCardInstance *) object);
    return 0;
}
