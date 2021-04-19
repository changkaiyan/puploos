
SECTIONS { 
  
    . = 0x3000000 ;
  .  =ALIGN(4) ;
.text : {  *.o(.text*); }
.  =ALIGN(4) ;
.data : {*(.data) ; *(.sdata) ; *(.init_array);}
.rodata : { *(.rodata*); }
.bss : { *(.sbss); *(.bss);}
}