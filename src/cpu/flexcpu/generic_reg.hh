/*
 * Copyright (c) 2018 The Regents of The University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Bradley Wang
 */

#include <type_traits>
#include <typeinfo>

#include "arch/registers.hh"
#include "config/the_isa.hh"
#include "cpu/reg_class.hh"

#ifndef __CPU_FLEXCPU_GENERIC_REG_HH__
#define __CPU_FLEXCPU_GENERIC_REG_HH__

/**
 * Generic register value container. Useful for storing results of instructions
 * or creating unified data structures for holding register values, without
 * needing to know the type of register at compile-time.
 */
class GenericReg {
  protected:
    using VecRegContainer = TheISA::VecRegContainer;
    using VecElem = TheISA::VecElem;
    using VecPredRegContainer = TheISA::VecPredRegContainer;

    /**
     * Union storage type, so that GenericReg is a fixed-size class. This
     * allows simple construction of vectors/arrays/etc. of GenericReg.
     */
    union GenericRegVal
    {
        RegVal baseVal;
        VecRegContainer vecReg;
        VecElem vecElem;
        VecPredRegContainer vecPredReg;
    } _value = {0};
    RegClass _regClass; // Used for enforcement, as well as identification.

  public:
    template<typename T>
    explicit GenericReg(const T& value, const RegClass reg_class):
        _regClass(reg_class)
    {
        switch(_regClass) {
          case RegClass::IntRegClass:
          case RegClass::FloatRegClass:
          case RegClass::CCRegClass:
          case RegClass::MiscRegClass:
            _value.baseVal = value;
            break;
          case RegClass::VecRegClass:
            panic("Templated call should not be used for VecRegClass!");
            break;
          case RegClass::VecElemClass:
            _value.vecElem = value;
            break;
          case RegClass::VecPredRegClass:
            panic("Templated call should not be used for VecPredRegClass!");
            break;
          default:
            panic("Tried to create GenericReg with unknown class!");
        }
    }

    explicit GenericReg(const VecRegContainer& value,
                        const RegClass reg_class):
        _regClass(RegClass::VecRegClass)
    {
        assert(reg_class == VecRegClass);
        _value.vecReg = value;
    }

    explicit GenericReg(const VecElem& value, const RegClass reg_class):
        _regClass(RegClass::VecElemClass)
    {
        assert(reg_class == RegClass::VecElemClass);
        _value.vecElem = value;
    }

    explicit GenericReg(const VecPredRegContainer& value,
                        const RegClass reg_class):
        _regClass(RegClass::VecPredRegClass)
    {
        assert(reg_class == RegClass::VecPredRegClass);
        _value.vecPredReg = value;
    }

    GenericReg(const GenericReg& other):
        _regClass(other.getRegClass())
    {
        switch(_regClass) {
          case RegClass::IntRegClass:
          case RegClass::FloatRegClass:
          case RegClass::CCRegClass:
          case RegClass::MiscRegClass:
            _value.baseVal = other._value.baseVal;
            break;
          case RegClass::VecRegClass:
            _value.vecReg = other.asVecReg();
            break;
          case RegClass::VecElemClass:
            _value.vecElem = other.asVecElem();
            break;
          case RegClass::VecPredRegClass:
            _value.vecPredReg = other.asVecPredReg();
            break;
          default:
            panic("Tried to copy a GenericReg with unknown type!");
        }
    }

    RegClass getRegClass() const
    {
        return _regClass;
    }

    RegVal& asIntReg()
    {
        assert(getRegClass() == IntRegClass);
        return _value.baseVal;
    }

    const RegVal& asIntReg() const
    {
        assert(getRegClass() == IntRegClass);
        return _value.baseVal;
    }

    RegVal& asFloatRegBits()
    {
        assert(getRegClass() == FloatRegClass);
        return _value.baseVal;
    }

    const RegVal& asFloatRegBits() const
    {
        assert(getRegClass() == FloatRegClass);
        return _value.baseVal;
    }

    VecRegContainer& asVecReg()
    {
        assert(getRegClass() == VecRegClass);
        return _value.vecReg;
    }

    const VecRegContainer& asVecReg() const
    {
        assert(getRegClass() == VecRegClass);
        return _value.vecReg;
    }

    VecElem& asVecElem()
    {
        assert(getRegClass() == VecElemClass);
        return _value.vecElem;
    }

    const VecElem& asVecElem() const
    {
        assert(getRegClass() == VecElemClass);
        return _value.vecElem;
    }

    VecPredRegContainer& asVecPredReg()
    {
        assert(getRegClass() == VecPredRegClass);
        return _value.vecPredReg;
    }

    const VecPredRegContainer asVecPredReg() const
    {
        assert(getRegClass() == VecPredRegClass);
        return _value.vecPredReg;
    }

    RegVal& asCCReg()
    {
        assert(getRegClass() == CCRegClass);
        return _value.baseVal;
    }

    const RegVal& asCCReg() const
    {
        assert(getRegClass() == CCRegClass);
        return _value.baseVal;
    }

    RegVal& asMiscReg()
    {
        assert(getRegClass() == MiscRegClass);
        return _value.baseVal;
    }

    const RegVal& asMiscReg() const
    {
        assert(getRegClass() == MiscRegClass);
        return _value.baseVal;
    }

    GenericReg& operator=(const GenericReg& other)
    {
        _regClass = other.getRegClass();
        switch(_regClass) {
          case RegClass::IntRegClass:
          case RegClass::FloatRegClass:
          case RegClass::CCRegClass:
          case RegClass::MiscRegClass:
            _value.baseVal = other._value.baseVal;
            break;
          case RegClass::VecRegClass:
            _value.vecReg = other.asVecReg();
            break;
          case RegClass::VecElemClass:
            _value.vecElem = other.asVecElem();
            break;
          case RegClass::VecPredRegClass:
            _value.vecPredReg = other.asVecPredReg();
            break;
          default:
            panic("Tried to copy a GenericReg with unknown type!");
        }
        return *this;
    }

    template<typename T>
    GenericReg& set(const T& value, const RegClass reg_class)
    {
        _regClass = reg_class;
        switch(_regClass) {
          case RegClass::IntRegClass:
          case RegClass::FloatRegClass:
          case RegClass::CCRegClass:
          case RegClass::MiscRegClass:
            _value.baseVal = value;
            break;
          case RegClass::VecRegClass:
            panic("Templated call should not be used for VecRegClass!");
            break;
          case RegClass::VecElemClass:
            _value.vecElem = value;
            break;
          case RegClass::VecPredRegClass:
            panic("Templated call should not be used for VecPredRegClass!");
            break;
          default:
            panic("Tried to set a GenericReg with unknown type!");
        }
        return *this;
    }

    GenericReg& set(const VecRegContainer& value, const RegClass reg_class)
    {
        assert(reg_class == RegClass::VecRegClass);
        _regClass = RegClass::VecRegClass;
        _value.vecReg = value;

        return *this;
    }

    GenericReg& set(const VecPredRegContainer& value, const RegClass reg_class)
    {
        assert(reg_class == RegClass::VecPredRegClass);
        _regClass = RegClass::VecPredRegClass;
        _value.vecPredReg = value;

        return *this;
    }
};

#endif // __CPU_FLEXCPU_GENERIC_REG_HH__
