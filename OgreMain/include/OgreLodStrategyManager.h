/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef __Lod_Strategy_Manager_H__
#define __Lod_Strategy_Manager_H__

#include "OgrePrerequisites.h"

#include "OgreSingleton.h"
#include "OgreHeaderPrefix.h"

namespace Ogre {
    template <typename T> class MapIterator;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup LOD
    *  @{
    */
    /** Manager for LOD strategies. */
    class _OgreExport LodStrategyManager : public Singleton<LodStrategyManager>, public LodAlloc
    {
        /** Map of strategies. */
        typedef std::map<String, LodStrategy *> StrategyMap;

        /** Internal map of strategies. */
        StrategyMap mStrategies;

        /** Default strategy. */
        LodStrategy *mDefaultStrategy;

    public:
        /** Default constructor. */
        LodStrategyManager();

        /** Destructor. */
        ~LodStrategyManager();

        /** Add a strategy to the manager. */
        void addStrategy(LodStrategy *strategy);

        /** Remove a strategy from the manager with a specified name.

            The removed strategy is returned so the user can control
            how it is destroyed.
        */
        LodStrategy *removeStrategy(const String& name);

        /** Remove and delete all strategies from the manager.

            All strategies are deleted.  If finer control is required
            over strategy destruction, use removeStrategy.
        */
        void removeAllStrategies();

        /** Get the strategy with the specified name. */
        LodStrategy *getStrategy(const String& name);

        /** Set the default strategy. */
        void setDefaultStrategy(LodStrategy *strategy);

        /** Set the default strategy by name. */
        void setDefaultStrategy(const String& name);

        /** Get the current default strategy. */
        LodStrategy *getDefaultStrategy();

        /** Get an iterator for all contained strategies. */
        MapIterator<StrategyMap> getIterator();

        /// @copydoc Singleton::getSingleton()
        static LodStrategyManager& getSingleton(void);
        /// @copydoc Singleton::getSingleton()
        static LodStrategyManager* getSingletonPtr(void);
    };
    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
