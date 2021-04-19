
SECTIONS { 
  
    . = 0x00 ;
  .  =ALIGN(4) ;
.text : {  *.o(.text); }
.  =ALIGN(4) ;
.data : {*(.data) ;*(.rodata) ; *(.sdata) ;}

}