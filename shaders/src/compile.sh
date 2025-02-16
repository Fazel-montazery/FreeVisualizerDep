#!/usr/bin/env bash

#Copyright (C) 2024 Caleb Cornett <caleb.cornett@outlook.com>

for filename in *.vert.hlsl; do
    if [ -f "$filename" ]; then
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/SPIRV/${filename/.hlsl/.spv}"
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/MSL/${filename/.hlsl/.msl}"
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.frag.hlsl; do
    if [ -f "$filename" ]; then
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/SPIRV/${filename/.hlsl/.spv}"
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/MSL/${filename/.hlsl/.msl}"
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/DXIL/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.comp.hlsl; do
    if [ -f "$filename" ]; then
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/SPIRV/${filename/.hlsl/.spv}"
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/MSL/${filename/.hlsl/.msl}"
        ../SDL_shadercross/build/shadercross "$filename" -o "../out/DXIL/${filename/.hlsl/.dxil}"
    fi
done
