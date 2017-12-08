// ************************************************************************** //
//
//  Steca2: stress and texture calculator
//
//! @file      core/typ/factory.h
//! @brief     Defines and implements templated class Factory
//!
//! @homepage  https://github.com/scgmlz/Steca2
//! @license   GNU General Public License v3 or higher (see COPYING)
//! @copyright Forschungszentrum Jülich GmbH 2017
//! @authors   Scientific Computing Group at MLZ (see CITATION, MAINTAINER)
//
// ************************************************************************** //

#ifndef FACTORY_H
#define FACTORY_H

#include "def/numbers.h"
#include "def/special_pointers.h"
#include "typ/exception.h"
#include "typ/hash.h"
#include "typ/str.h"

namespace typ {

template <class ProductBase> class Factory {
public:
    struct MakerBase {
        virtual ~MakerBase() {}
        virtual owner_not_null<ProductBase*> make() = 0;
    };

    template <class Product> struct Maker : MakerBase {
        owner_not_null<ProductBase*> make() {
            return owner_not_null<ProductBase*>::from(new Product);
        }
    };

private:
    owning_hash<str, MakerBase*> makers_;

public:
    virtual ~Factory() {}

    void addMaker(rcstr key, owner_not_null<MakerBase*> maker) { makers_.insert(key, maker); }

    owner_not_null<ProductBase*> make(rcstr key) THROWS {
        MakerBase* maker = makers_.value(key);
        RUNTIME_CHECK(maker, "no maker " % key);
        return maker->make();
    }
};

} // namespace typ

// code to call only once - guard
#define ONLY_ONCE                                                                                  \
    {                                                                                              \
        static bool once = false;                                                                  \
        if (once)                                                                                  \
            return;                                                                                \
        once = true;                                                                               \
    }

#endif // FACTORY_H
