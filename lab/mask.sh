#!/usr/bin/env bash
# Mascara segredos conhecidos em evidências (stdin -> stdout).
sed -E \
  -e 's/readwrite/****/g' \
  -e 's/NXH-LAB-[A-Z0-9-]+/NXH-****/g'
