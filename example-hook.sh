#!/usr/bin/env bash

case $1 in
  lease) # when a lease happens
    echo $@ >> /tmp/hook.log
    ;;
  release) # when a client requests a release or a lease expires
    echo $@ >> /tmp/hook.log
    ;;
  endlearning) # once the initial timeout for gathering all blocks ends
    echo "$@ - learning phase is over" >> /tmp/hook.log
    ;;
  # if you want to propagate addresses in some way in the network, those two hooks can be used for that
  claim) # after ddhcpd has claimed this particular address - you can also use lease|claim)
    echo $@ >> /tmp/hook.log
    ;;
  claimrelease) # before this block is no logner claimed - you can also use release|claimrelease)
    echo $@ >> /tmp/hook.log
    ;;
esac
