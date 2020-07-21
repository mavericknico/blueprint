/*
  ==============================================================================

    blueprint_ShadowView.h
    Created: 17 Apr 2019 8:38:37am

  ==============================================================================
*/

#pragma once

#include "blueprint_View.h"


namespace blueprint
{

    struct BoundsAnimator : public juce::Timer {
        using StepCallback = std::function<void(juce::Rectangle<float>)>;

        juce::Rectangle<float> start;
        juce::Rectangle<float> dest;
        StepCallback callback;
        double duration;
        double startTime;

        BoundsAnimator(double durationMs, juce::Rectangle<float> start, juce::Rectangle<float> dest, StepCallback cb)
            : start(start)
            , dest(dest)
            , callback(cb)
            , duration(durationMs)
        {
            startTime = juce::Time::getMillisecondCounterHiRes();
            startTimerHz(45);
        }

        ~BoundsAnimator() {
            callback(dest);
            stopTimer();
        }

        float lerp(float a, float b, float t) {
            return a + (t * (b - a));
        }

        void timerCallback() override {
            double const now = juce::Time::getMillisecondCounterHiRes();
            double t = std::clamp((now - startTime) / duration, 0.0, 1.0);

            // TODO: We can use different types of easing here by modifying t
            // before `lerp`. E.g. for quadratic ease-in-out:
            t = (t < 0.5) ? (2.0 * t * t) : (-1.0 + (4.0 - 2.0 * t) * t);
            t = std::clamp(t, 0.0, 1.0);

            if (t >= 0.9999) {
                callback(dest);
                stopTimer();
                return;
            }

            callback({
                lerp(start.getX(), dest.getX(), t),
                lerp(start.getY(), dest.getY(), t),
                lerp(start.getWidth(), dest.getWidth(), t),
                lerp(start.getHeight(), dest.getHeight(), t),
            });
        }
    };

    //==============================================================================
    /** The ShadowView class decouples layout constraints from the actual View instances
        so that our View tree and ShadowView tree might differ (i.e. in the case of raw
        text nodes), and so that our View may remain more simple.
     */
    class ShadowView
    {
    public:
        //==============================================================================
        ShadowView(View* _view) : view(_view)
        {
            YGConfigSetUseWebDefaults(YGConfigGetDefault(), true);
            yogaNode = YGNodeNew();
        }

        virtual ~ShadowView()
        {
            YGNodeFree(yogaNode);
        }

        //==============================================================================
        /** Set a property on the shadow view. */
        virtual void setProperty (const juce::Identifier& name, const juce::var& newValue);

        /** Adds a child component behind the existing children. */
        virtual void addChild (ShadowView* childView, int index = -1)
        {
            if (index == -1)
            {
                YGNodeInsertChild(yogaNode, childView->yogaNode, YGNodeGetChildCount(yogaNode));
                children.push_back(childView);
            }
            else
            {
                jassert (juce::isPositiveAndNotGreaterThan(index, YGNodeGetChildCount(yogaNode)));

                YGNodeInsertChild(yogaNode, childView->yogaNode, index);
                children.insert(children.begin() + index, childView);
            }
        }

        /** Removes a child component from the children array. */
        virtual void removeChild (ShadowView* childView)
        {
            auto it = std::find(children.begin(), children.end(), childView);

            if (it != children.end())
            {
                YGNodeRemoveChild(yogaNode, childView->yogaNode);
                children.erase(it);
            }
        }

        //==============================================================================
        /** Returns a pointer to the View instance shadowed by this node. */
        View* getAssociatedView() { return view; }

        /** Returns the layout bounds held by the internal yogaNode. */
        juce::Rectangle<float> getCachedLayoutBounds()
        {
            return {
                YGNodeLayoutGetLeft(yogaNode),
                YGNodeLayoutGetTop(yogaNode),
                YGNodeLayoutGetWidth(yogaNode),
                YGNodeLayoutGetHeight(yogaNode)
            };
        }

        /** Recursively computes the shadow tree layout. */
        void computeViewLayout(const float width, const float height)
        {
            // Compute the new layout values
            YGNodeCalculateLayout(yogaNode, width, height, YGDirectionInherit);
        }

        /** Recursive traversal of the shadow tree, flushing layout bounds to
            the associated view components.
         */
        virtual void flushViewLayout(bool animated = false)
        {
#ifdef DEBUG
            if (props.contains("debug"))
                YGNodePrint(yogaNode, (YGPrintOptions) (YGPrintOptionsLayout
                                                        | YGPrintOptionsStyle
                                                        | YGPrintOptionsChildren));
#endif

            if (animated || props.getWithDefault("animated", false))
            {
                auto viewCurrentBounds = view->getBounds().toFloat();
                auto viewDestinationBounds = getCachedLayoutBounds();

                animator = std::make_unique<BoundsAnimator>(
                    200.0,
                    viewCurrentBounds,
                    viewDestinationBounds,
                    [safeView = juce::Component::SafePointer(view)](auto && stepBounds) {
                        if (auto* view = safeView.getComponent()) {
                            view->setFloatBounds(stepBounds);
                            view->setBounds(stepBounds.toNearestInt());
                        }
                    }
                );
            }
            else
            {
                view->setFloatBounds(getCachedLayoutBounds());
                view->setBounds(getCachedLayoutBounds().toNearestInt());
            }

            for (auto& child : children)
                child->flushViewLayout(animated);
        }

    protected:
        //==============================================================================
        YGNodeRef yogaNode;
        View* view = nullptr;
        juce::NamedValueSet props;

        std::unique_ptr<BoundsAnimator> animator;
        std::vector<ShadowView*> children;

    private:
        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadowView)
    };

}
