#!/bin/sh
ipfw -q delete 103
ipfw -q delete 203
ipfw -q delete 303
ipfw -q delete 403
ipfw -q delete 503
ipfw -q delete 603
ipfw -q delete 703
ipfw -q delete 803
ipfw -q delete 903

#ipfw -q add 103 prob 0.5 divert 2000 sctp from 10.1.1.0/24 to any in
#ipfw -q add 203 prob 0.5 divert 2000 sctp from 10.1.2.0/24 to any in
ipfw -q add 303 prob 0.2 divert 2000 sctp from 10.1.3.0/24 to any in
ipfw -q add 403 prob 0.2 divert 2000 sctp from 10.1.4.0/24 to any in
#ipfw -q add 503 prob 0.5 divert 2000 sctp from 10.1.5.0/24 to any in
#ipfw -q add 603 prob 0.5 divert 2000 sctp from 10.1.6.0/24 to any in
#ipfw -q add 703 prob 0.5 divert 2000 sctp from 10.1.7.129/32 to any in
#ipfw -q add 803 prob 0.5 divert 2000 sctp from 10.1.8.0/24 to any in
#ipfw -q add 903 prob 0.5 divert 2000 sctp from 10.1.9.0/24 to any in
