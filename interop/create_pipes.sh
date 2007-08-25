#!/bin/sh
ipfw -q delete 100
ipfw -q delete 101
ipfw -q delete 200
ipfw -q delete 201
ipfw -q delete 300
ipfw -q delete 301
ipfw -q delete 400
ipfw -q delete 401
ipfw -q delete 500
ipfw -q delete 501
ipfw -q delete 600
ipfw -q delete 601
ipfw -q delete 700
ipfw -q delete 701
ipfw -q delete 800
ipfw -q delete 801
ipfw -q delete 900
ipfw -q delete 901

ipfw -q add 100 prob 0.5 pipe 1 ip from 10.1.1.0/24 to any in
ipfw -q add 101 pipe 2 ip from 10.1.1.0/24 to any in
ipfw -q add 200 prob 0.5 pipe 3 ip from 10.1.2.0/24 to any in
ipfw -q add 201 pipe 4 ip from 10.1.2.0/24 to any in
ipfw -q add 300 prob 0.5 pipe 5 ip from 10.1.3.0/24 to any in
ipfw -q add 301 pipe 6 ip from 10.1.3.0/24 to any in
ipfw -q add 400 prob 0.5 pipe 7 ip from 10.1.4.0/24 to any in
ipfw -q add 401 pipe 8 ip from 10.1.4.0/24 to any in
ipfw -q add 500 prob 0.5 pipe 9 ip from 10.1.5.0/24 to any in
ipfw -q add 501 pipe 10 ip from 10.1.5.0/24 to any in in
ipfw -q add 600 prob 0.5 pipe 11 ip from 10.1.6.0/24 to any in
ipfw -q add 601 pipe 12 ip from 10.1.6.0/24 to any in
ipfw -q add 700 prob 0.5 pipe 13 ip from 10.1.7.0/24 to any in
ipfw -q add 701 pipe 14 ip from 10.1.7.0/24 to any in
ipfw -q add 800 prob 0.5 pipe 15 ip from 10.1.8.0/24 to any in
ipfw -q add 801 pipe 16 ip from 10.1.8.0/24 to any in
ipfw -q add 900 prob 0.5 pipe 17 ip from 10.1.9.0/24 to any in
ipfw -q add 901 pipe 17 ip from 10.1.9.0/24 to any in
