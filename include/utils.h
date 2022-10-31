/***
    @author: Wangzhiming
    @date: 2021-10-29
***/
#pragma once
#define DISALLOW_COPY_MOVE_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete;        \
    TypeName(const TypeName &&) = delete;       \
    TypeName &operator=(const TypeName &) = delete
