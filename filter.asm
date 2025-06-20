;-;to compile - use the folowing command:
;       nasm -f win32 filter.asm -o filter.obj
section .data
    ; Arrays of float32 (4 bytes each)
    ;NOTE: addresses in NASM placed so:
      ;myArray  db  addr, addr+1, addr+2, addr+3, .....  
    align 16
    y_array dd 0.0, 0.0, 0.0, 0.0         
    x_array dd 0.0, 0.0, 0.0, 0.0
    f32temp_store dd 0.0       
    ; xmm0-inputs , xmm1-outputs
    ; xmm2-x_coefs , xmm3-y_coefs
    ;a0-a3 input coeficients, b1-b3 output coefs
    ; output = a0*[x] + a1*[x-1] + a2*[x-2] +a3*[x-3] - b1*[y-1] - b2*[y-2] - b3*[y-3];
    ;NOTE: In case of the DPPS instruction b coefficients should be made changed sign 
    zero_float dd 0.000000000
    ; 32-bit variables
    i32var01 dd 0
    i32var02 dd 0
 
 
    ; constants a0, a1, b0=0.0, b1
      ;NOTE: addresses in NASM placed so:
      ;myArray  db  addr, addr+1, addr+2, addr+3, ..... 
    a_constants dd 0.00041655, 0.00124965, 0.00124965, 0.00041655 ;input coefs
    b_constants dd 0.0, 2.68615740, -2.41965511, 0.73016535  ;output coefs

 ;--------------CIC---variables--begin
     comb1Array resb 64  ;circular arrays 16 cells int32
     comb2Array resb 64
     comb1Ptr dd 0    ; a part of pointer (low 4 bits)
     comb2Ptr dd 0
     acc1 dd 0
     acc2 dd 0
     ;-----------------CIC--end----------

section .text
    global _filter_proc, _word_to_dword, _dword_to_word, _cic16ord2

_filter_proc:
     ;  call in C program so:   filter_proc(uint_32t src_addr, desat_addr, amount_of_data)
      push ebp
      mov ebp, esp
      %define src_addr  [ebp+8]  ;source array
      %define dest_addr [ebp+12]  ;destinatoin array
      %define amount_of_data [ebp+16] ;amount of data to process
      ;load pointers
      mov esi, src_addr
      mov edi, dest_addr
      mov ecx, amount_of_data
one_sample_iteration:
    ; 1) Load data in i32var1
       mov eax, [esi]
       mov [i32var01], eax       ; store to i32var1

    ; 2) Load i32var1 as float on FPU stack
       lea ebx, [i32var01]         ; load address of i32var1 into EBX
       fild dword [ebx]         ; load int32 as float into ST(0)
    ; 3) Load data from input x_array into xmm0 
       movups xmm0, [x_array]
       
    ; 4) Shift left input samples (are in xmm0) , like this: [x2,x0,x1,zero]
       shufps xmm0, xmm0, 0x90
       
    ; 5) Store ST(0) float (input sample converted) to f32temp_store
       fstp dword [f32temp_store]     ; store ST(0) float and pop FPU stack
    ; 6)apply a  new input sample to the first float in XMM0: (xmm0[0] = f32temp_store)
      insertps xmm0, [f32temp_store], 0

    ; 6.1) Store input samples in RAM
       movups [x_array], xmm0
    
    ; 7) Load outputs  into xmm1: [0.0 | 0.0 | b1 | b2]
       movups xmm1, [y_array]
     
    ; 8) Load  constants "a", "b" into  xmm2, xmm3
       movups  xmm2, [a_constants]
       movups xmm3,  [b_constants]

    ; 9) Multiply and sum products of inputs by DPPS xmm0, xmm1, 0xf1
    ; 0xf1 means mask to keep only lowest float of destination and sum all four products
      dpps xmm0, xmm2, 0xf1
      
      
    ; 10) Multiply and sum products for outputs
      dpps xmm3, xmm1, 0xf1
  
    ; 11) output of filter = (a0*[x]+a1*[x-1]+a2*[x-2]) with (b1*[y-1]+b2*[y-2])
      addss xmm0, xmm3   ;add input and output products
      
    ; 12) Shift left xmm1 (output array) and assign new output data to the second cell:
      shufps xmm1, xmm1, 0x90  ;shift left [old|y2|y1|0]
      insertps xmm1, xmm0, 0b00010000 ;assign new filter output   xmm1[1]=xmm0[0]
    ; 13)Save output array in RAM:
      movups [y_array], xmm1
      
    ; 14) Convert xmm0 float to signed 32-bit int in EAX (OK runs correctly)
      cvttss2si eax, xmm0

     ;15) move a new result to output array
     mov [edi], eax
     ;16) increment source and destination
     add esi, 4
     add edi, 4
     ;go to the next input value
     loop  one_sample_iteration 
    ; 10) Result in AX, ready to return
    
      pop ebp

    ret

    ;-----extends 16 to 32bit
_word_to_dword:
        ;   call in C program so:   word_to_dword(uint_32t src_addr, desat_addr, amount_of_data)
      push ebp
      mov ebp, esp
      %define src_addr1  [ebp+8]   ;source array (16bit)
      %define dest_addr1 [ebp+12]  ;destinatoin array (32bit)
      %define amount_of_data1 [ebp+16] ;amount of data to process
      mov esi, src_addr1
      mov edi, dest_addr1
      mov ecx, amount_of_data1
      mov eax, 0x00000000
innerLoop_w_to_d:
      movsx  eax, word [esi]
      mov  [edi], eax
      add esi, 2
      add edi, 4
      loop innerLoop_w_to_d

      pop ebp
      ret
;--truncate 32bit to 16bit
   
_dword_to_word:
        ;   call in C program so:   dword_to_word(uint_32t src_addr, desat_addr, amount_of_data)
      push ebp
      mov ebp, esp
      %define src_addr2  [ebp+8]   ;source array (16bit)
      %define dest_addr2 [ebp+12]  ;destinatoin array (32bit)
      %define amount_of_data2 [ebp+16] ;amount of data to process
      mov esi, src_addr2
      mov edi, dest_addr2
      mov ecx, amount_of_data2
      mov eax, 0x00000000
innerLoop_d_to_w:
      mov  eax,  [esi]
      mov  [edi], ax
      add esi, 4
      add edi, 2
      loop innerLoop_d_to_w  
      pop ebp
      ret
;cic16ord2(uint_32t src_addr, desat_addr, amount_of_data)
 _cic16ord2:
  ;---C style call conversion
  %define cicSrc [ebp+8]
  %define cicDest [ebp+12]
  %define cicLength [ebp+16]
  %define resultValue [ebp-4]
  %define array_size 256
   ;[ebp] - return address
  push ebp
  mov ebp, esp
  sub esp, 4 ;allocate local variable
  ;load esi,edi,ecx. !!!! ECX reserved for loop
  mov esi, cicSrc
  mov edi, cicDest
  mov ecx, cicLength
  ;2)init pointers to circular arrays
  
cic_loop_start:
  ;3) Add input data to the acc1, save result there.After this, do it: acc2=Acc1+acc2
    mov eax, [esi] ;load input data from array
    mov ebx, [acc1]
    add eax, ebx      ;acc1+=input
    mov [acc1], eax   ;save acc1
    mov ebx, [acc2]
    add eax, ebx      ;acc2+=acc1
    mov [acc2], eax   ;save acc2
  ;4)COMB1 Load data by comb1Ptr pointer, then sutract it from acc2.Save result by the comb1Ptr pointer
    mov eax, comb1Array ;adress of first cell
    mov ebx, [comb1Ptr] ;offset
    add eax, ebx        ;address exectly
    mov edx, eax        ;copy address
    mov eax, [eax]      ;data of array`s item
    mov ebx, [acc1]
    sub ebx, eax  ;ebx=acc2-[comb1Ptr]
    mov [edx], ebx ;save result by a pointer value into array
    mov resultValue, ebx
  ;5)Increment the pointer combPtr1, when it is more that zero_array_cell+64, wrap it back
    mov ebx, [comb1Ptr]  ;offset of pointer
    add ebx, 4  ;next cell
    and ebx, 0x0000003f ;maximum value = 15, othervise wrap around
    mov [comb1Ptr], ebx ;store offset of pointer
  ;6)COMB2. subtract delayed value, loaded by comb2Ptr from resultValue.The result store again by the poiner
    mov eax, comb2Array ;adress of first cell
    mov ebx, [comb2Ptr] ;offset
    add eax, ebx        ;address exectly
    mov edx, eax        ;copy address
    mov eax, [eax]      ;data of array`s item
    mov ebx, [acc2]
    sub ebx, eax  ;ebx=acc2-[comb1Ptr]
    mov [edx], ebx ;save result by a pointer value into array
    mov resultValue, ebx
  ;7)Increment the pointer combPtr2, when it is more that zero_array_cell+64, wrap it back
    mov ebx, [comb2Ptr]  ;offset of pointer
    add ebx, 4  ;next cell
    and ebx, 0x0000003f ;maximum value = 15, othervise wrap around
    mov [comb2Ptr], ebx ;store offset of pointer
    mov eax, resultValue
    mov [edi], eax ;debug:save to ouput array
    ;8)increment registers ESI EDI
    add esi, 4
    add edi, 4
    dec ecx
    jnz cic_loop_start
    ;--restore stack
    add esp, 4
    pop ebp
    ret
  
