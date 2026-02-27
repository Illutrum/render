#!/usr/bin/env bash

cmake .
make

cd js
  npm install
  npm run build
cd ..
