#!/bin/bash
basepath=$(cd `dirname $0`; pwd)
ulimit -c unlimited

$basepath/../bin/ConnTestClient 127.0.0.1 9000 100
