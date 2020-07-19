/*
  ==============================================================================

    blueprint_AnimatedShadowView.h
    Created: 3 Marc 2020 7:29:37pm

  ==============================================================================
*/

#pragma once

#include "blueprint_ShadowView.h"
#include "blueprint_View.h"


namespace blueprint
{

    //==============================================================================
    /** The AnimatedShadowView is a very simple ShadowView implementation that flushes
     *  layout bounds to its associated view over time instead of instantaneously.
     */
    class AnimatedShadowView : public ShadowView
    {
    public:
        //==============================================================================
        AnimatedShadowView(View* _view)
            : ShadowView(_view) {}

        //==============================================================================
        void flushViewLayout() override
        {
            auto& animator = juce::Desktop::getInstance().getAnimator();
            auto bounds = getCachedLayoutBounds();

            // TODO: This is kind of hacky; to do this correctly we should call
            // setFloatBounds frequently with the same interpolated bounds that the
            // animator delegates to the view. This will only really cause issues when
            // the view wants to paint something with knowledge of its float bounds (like
            // trying to center a procedural painting). In that case you can use the
            // proxy component flag to take a snapshot before beginning the animation, but
            // it's kind of hacky.
            view->setFloatBounds(bounds);
            animator.animateComponent(view, bounds.toNearestInt(), 1.0f, 150, false, 1.0, 1.0);

            for (auto& child : children)
                child->flushViewLayout();
        }

    private:
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimatedShadowView)
    };

}
