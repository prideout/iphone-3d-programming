#pragma once
#include "Vector.hpp"

template <typename T>
struct Box2 {
    
    static Box2<T> FromLeftTopRightBottom(T left, T top, T right, T bottom)
    {
        Box2<T> box;
        box.x = left;
        box.y = top;
        box.width = right - left;
        box.height = bottom - top;
        return box;
    }
    
    const T* Pointer() const
    {
        return &x;
    }
    
    void FlipY(T total)
    {
        y = total - y - height;
    }
    
    T x;
    T y;
    T width;
    T height;
};

template <typename T>
struct Box3 {
    
    static Box3<T> FromCenterBox(const Vector2<T>& center, const Box2<T>& source)
    {
        Box3<T> box;
        box.width = source.width;
        box.height = source.height;
        box.x = center.x - box.width / 2;
        box.y = center.y - box.height / 2;;
        box.z = 0;
        return box;
    }
    
    const T* Pointer() const
    {
        return &x;
    }
    T x;
    T y;
    T z;
    T width;
    T height;
};

typedef Box2<float> box2;
typedef Box3<float> box3;
