#!/bin/bash

cd build
./cmoe GET "name=fumiama&reg=fumiama"
./cmoe GET "name=fumiam&reg=fumiama"
./cmoe GET "name=fumia&reg=fumiama"
./cmoe GET "name=fumi&reg=fumiama"
./cmoe GET "name=fum&reg=fumiama"
./cmoe GET "name=fu&reg=fumiama"
./cmoe GET "name=f&reg=fumiama"

for ((i=0; i<$1; i++))
do
  ./cmoe GET "name=fumiama&theme=r34" &
  ./cmoe GET "name=fumiam&theme=r34" &
  ./cmoe GET "name=fumia&theme=r34" &
  ./cmoe GET "name=fumi&theme=r34" &
  ./cmoe GET "name=fum&theme=r34" &
  ./cmoe GET "name=fu&theme=r34" &
  ./cmoe GET "name=f&theme=r34" &
done
