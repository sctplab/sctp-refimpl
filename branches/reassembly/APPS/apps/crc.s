0x804846c <update_crc32>:       push   %ebp
0x804846d <update_crc32+1>:     mov    %esp,%ebp
0x804846f <update_crc32+3>:     push   %edi
0x8048470 <update_crc32+4>:     push   %esi
0x8048471 <update_crc32+5>:     push   %ebx
0x8048472 <update_crc32+6>:     mov    0x8(%ebp),%edx   # get crc32 -> edx
0x8048475 <update_crc32+9>:     mov    0xc(%ebp),%edi   # get buffer -> edi
0x8048478 <update_crc32+12>:    mov    0x10(%ebp),%ebx  # get lengthx -> ebx
0x804847b <update_crc32+15>:    xor    %ecx,%ecx        # zero out ecx
0x804847d <update_crc32+17>:    cmp    %ebx,%ecx        # are we done
0x804847f <update_crc32+19>:    jae    0x804849d <update_crc32+49>  # if so go to 49
0x8048481 <update_crc32+21>:    mov    $0x8049580,%esi  # move index of table to esi
0x8048486 <update_crc32+26>:    mov    %esi,%esi        # fill instruction for fall through case
0x8048488 <update_crc32+28>:    mov    %edx,%eax        # move crc to eax
0x804848a <update_crc32+30>:    shr    $0x8,%eax        # shift right 8 eax
0x804848d <update_crc32+33>:    xor    (%ecx,%edi,1),%dl  # xor buffer[i] and dl (edx)
0x8048490 <update_crc32+36>:    movzbl %dl,%edx           # and edx to get just bottom byte
0x8048493 <update_crc32+39>:    xor    (%esi,%edx,4),%eax # xor table entry into eax 
0x8048496 <update_crc32+42>:    mov    %eax,%edx          # copy eax into edx (new crc entry)
0x8048498 <update_crc32+44>:    inc    %ecx               # incr count
0x8048499 <update_crc32+45>:    cmp    %ebx,%ecx          # cmp count
0x804849b <update_crc32+47>:    jb     0x8048488 <update_crc32+28> # back up
0x804849d <update_crc32+49>:    mov    %edx,%eax      # done place result in eax
0x804849f <update_crc32+51>:    pop    %ebx
0x80484a0 <update_crc32+52>:    pop    %esi
0x80484a1 <update_crc32+53>:    pop    %edi
0x80484a2 <update_crc32+54>:    leave  
0x80484a3 <update_crc32+55>:    ret    
