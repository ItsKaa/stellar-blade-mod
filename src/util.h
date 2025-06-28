#pragma once
#include <safetyhook/context.hpp>
#include <spdlog/spdlog.h>

static void LogAllValues(const char* title, const safetyhook::Context& ctx)
{
    spdlog::debug(
        "{:s}, values: {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}, {:d}"
        , title
        , ctx.rax, ctx.rbx, ctx.rcx, ctx.rdx, ctx.rsi, ctx.rdi, ctx.rbp
        , ctx.rsp, ctx.r8, ctx.r9, ctx.r10, ctx.r11, ctx.r12, ctx.r13, ctx.r14, ctx.r15, ctx.rip
    );
}
