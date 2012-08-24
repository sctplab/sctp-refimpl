#!/bin/sh
ipfw -q delete 102
ipfw -q delete 202
ipfw -q delete 302
ipfw -q delete 402
ipfw -q delete 502
ipfw -q delete 602
ipfw -q delete 702
ipfw -q delete 802
ipfw -q delete 902

#ipfw -q add 102 prob 1.0 divert 2001 sctp from 10.1.1.0/24 to any in
#ipfw -q add 202 prob 1.0 divert 2001 sctp from 10.1.2.0/24 to any in
#ipfw -q add 302 prob 0.5 divert 2001 sctp from 10.1.3.0/24 to any in
#ipfw -q add 402 prob 0.5 divert 2001 sctp from 10.1.4.0/24 to any in
#ipfw -q add 502 prob 0.5 divert 2001 sctp from 10.1.5.0/24 to any in
#ipfw -q add 602 prob 0.5 divert 2001 sctp from 10.1.6.0/24 to any in
#ipfw -q add 702 prob 0.5 divert 2001 sctp from 10.1.7.0/24 to any in
#ipfw -q add 802 prob 0.5 divert 2001 sctp from 10.1.8.0/24 to any in
#ipfw -q add 902 prob 0.5 divert 2001 sctp from 10.1.9.0/24 to any in
