! Spring 2024 Revisions

! This program executes pow as a test program using the LC 3300 calling convention
! Check your registers ($v0) and memory to see if it is consistent with this program

! vector table
vector0:
        .fill 0x00000000                        ! device ID 0
        .fill 0x00000000                        ! device ID 1
        .fill 0x00000000                        ! ...
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000
        .fill 0x00000000                        ! device ID 7
        ! end vector table

main:	lea $sp, initsp                         ! initialize the stack pointer
        lw $sp, 0($sp)                          ! finish initialization

        addi $t0, $zero, -1                     ! Test shifts and FABs functionality (you can ignore this)
        fabs $t0                                
        addi $t1, $zero, 2                      
        srl $t1, $t0, $t1                       

        lea $t1, ticks                          ! Test additional shifts (you can ignore this)
        lw $t1, 0($t1)                                  
        addi $t0, $zero, 16                     !$t0 = 16
        ror $t1, $t1, $t0                       !$t1 = 0xFFFF0000



        lea $a0, timer_handler                 ! Install timer interrupt handler into vector table
        sw $a0, 0($zero)

        lea $a0, distance_tracker_handler     ! Install distance tracker interrupt handler into vector table
        sw $a0, 1($zero)


        lea $t0, minval
        lw $t0, 0($t0)
	lea $t1, INT_MAX 			! store 0x7FFFFFFF into minval (to initialize)
	lw $t1, 0($t1)
        sw $t1, 0($t0)

        ei                                      ! Enable interrupts

        lea $a0, BASE                           ! load base for pow
        lw $a0, 0($a0)
        lea $a1, EXP                            ! load power for pow
        lw $a1, 0($a1)
        lea $at, POW                            ! load address of pow
        jalr $at, $ra                           ! run pow
        lea $a0, ANS                            ! load base for pow
        sw $v0, 0($a0)

        halt                                    ! stop the program here
        addi $v0, $zero, -1                     ! load a bad value on failure to halt

BASE:   .fill 2
EXP:    .fill 8
ANS:	.fill 0                                 ! should come out to 256 (BASE^EXP)

INT_MAX: .fill 0x7FFFFFFF

POW:    addi $sp, $sp, -1                       ! allocate space for old frame pointer
        sw $fp, 0($sp)

        addi $fp, $sp, 0                        ! set new frame pointer

        bgt $a1, $zero, BASECHK                 ! check if $a1 is zero
        beq $zero, $zero, RET1                  ! if the exponent is 0, return 1

BASECHK:bgt $a0, $zero, WORK                    ! if the base is 0, return 0
        beq $zero, $zero, RET0

WORK:   addi $a1, $a1, -1                       ! decrement the power
        lea $at, POW                            ! load the address of POW
        addi $sp, $sp, -2                       ! push 2 slots onto the stack
        sw $ra, -1($fp)                         ! save RA to stack
        sw $a0, -2($fp)                         ! save arg 0 to stack
        jalr $at, $ra                           ! recursively call POW
        add $a1, $v0, $zero                     ! store return value in arg 1
        lw $a0, -2($fp)                         ! load the base into arg 0
        lea $at, MULT                           ! load the address of MULT
        jalr $at, $ra                           ! multiply arg 0 (base) and arg 1 (running product)
        lw $ra, -1($fp)                         ! load RA from the stack
        addi $sp, $sp, 2

        beq $zero, $zero, FIN                   ! unconditional branch to FIN

RET1:   add $v0, $zero, $zero                   ! return a value of 0
	addi $v0, $v0, 1                        ! increment and return 1
        beq $zero, $zero, FIN                   ! unconditional branch to FIN

RET0:   add $v0, $zero, $zero                   ! return a value of 0

FIN:	lw $fp, 0($fp)                          ! restore old frame pointer
        addi $sp, $sp, 1                        ! pop off the stack
        jalr $ra, $zero

MULT:   add $v0, $zero, $zero                   ! return value = 0
        addi $t0, $zero, 0                      ! sentinel = 0
AGAIN:  add $v0, $v0, $a0                       ! return value += argument0
        addi $t0, $t0, 1                        ! increment sentinel
        bgt $a1, $t0, AGAIN                     ! while sentinel < argument, loop again
        jalr $ra, $zero                         ! return from mult

timer_handler:
        addi $sp, $sp, -1
        sw $k0, 0($sp)

        ei 

        addi $sp, $sp, -1
        sw $s0, 0($sp)

        addi $sp, $sp, -1
        sw $s1, 0($sp)

        lea $s0, ticks
        lw $s0, 0($s0)
        lw $s1, 0($s0)
        addi $s1, $s1, 1
        sw $s1, 0($s0)

        lw $s1, 0($sp)
        addi $sp, $sp, 1

        lw $s0, 0($sp)
        addi $sp, $sp, 1

        di 

        lw $k0, 0($sp)
        addi $sp, $sp, 1

        reti

        

distance_tracker_handler:
        addi $sp, $sp, -1
        sw $k0, 0($sp)

        ei

        addi $sp, $sp, -1
        sw $a0, 0($sp)

        addi $sp, $sp, -1
        sw $a1, 0($sp)

        addi $sp, $sp, -1
        sw $a2, 0($sp)

        addi $sp, $sp, -1
        sw $t0, 0($sp)

        addi $sp, $sp, -1
        sw $t1, 0($sp)

        addi $sp, $sp, -1
        sw $t2, 0($sp)

        addi $sp, $sp, -1
        sw $s0, 0($sp)

        addi $sp, $sp, -1
        sw $s1, 0($sp)

        addi $sp, $sp, -1
        sw $s2, 0($sp)

        addi $sp, $sp, -1
        sw $fp, 0($sp)

        addi $sp, $sp, -1
        sw $ra, 0($sp)

        addi $sp, $sp, -1
        sw $at, 0($sp)

        addi $sp, $sp, -1
        sw $v0, 0($sp)

        in $t0, 1              ! store the value of distance tracker into t0

        lea $s0, maxval        
        lw $s1, 0($s0)         ! load the addr of maxval into s1
        lw $s2, 0($s1)         ! load the value of maxval in s2

        bgt $t0, $s2, bigger   ! if max is bigger than distTracker, GOTO notbigger

        lea $s0, minval
        lw $s1, 0($s0)
        lw $s2, 0($s1)

        bgt $s2, $t0, smaller
        beq $zero, $zero, final

smaller:
        sw $t0, 0($s1)
        beq $zero, $zero, updaterange1
        
bigger:
        sw $t0, 0($s1)                        ! update value of maxval

        lea $t1, minval                       ! edge case check
        lw $t1, 0($t1)                        ! t1 holds min addr        
        lw $t2, 0($t1)                        ! t2 holds minval

        bgt $t2, $t0, edge                    ! if minval > maxval, GOTO edge
        beq $zero, $zero, updaterange2

edge:
        sw $t0, 0($t1)                        ! store maxval into minval address
        beq $zero, $zero, updaterange2

updaterange1:                    ! t0 = min
        lea $t1, range           ! range
        lw $t1, 0($t1)
        lw $t2, 0($t1)

        lea $s0, maxval          ! max val
        lw $s0, 0($s0)
        lw $s0, 0($s0)

        nand $s0, $s0, $s0
        addi $s0, $s0, 1

        add $s0, $s0, $t0
        fabs $s0
        sw $s0, 0($t1)
        beq $zero, $zero, final

updaterange2:                    ! t0 = max
        lea $t1, range           ! range
        lw $t1, 0($t1)
        lw $t2, 0($t1)

        lea $s0, minval          ! min val
        lw $s0, 0($s0)
        lw $s0, 0($s0)

        nand $s0, $s0, $s0
        addi $s0, $s0, 1

        add $s0, $s0, $t0
        fabs $s0
        sw $s0, 0($t1)
        beq $zero, $zero, final

final:

        lw $v0, 0($sp)
        addi $sp, $sp, 1

        lw $at, 0($sp)
        addi $sp, $sp, 1

        lw $ra, 0($sp)
        addi $sp, $sp, 1

        lw $fp, 0($sp)
        addi $sp, $sp, 1

        lw $s2, 0($sp)
        addi $sp, $sp, 1

        lw $s1, 0($sp)
        addi $sp, $sp, 1

        lw $s0, 0($sp)
        addi $sp, $sp, 1

        lw $t2, 0($sp)
        addi $sp, $sp, 1

        lw $t1, 0($sp)
        addi $sp, $sp, 1

        lw $t0, 0($sp)
        addi $sp, $sp, 1

        lw $a2, 0($sp)
        addi $sp, $sp, 1

        lw $a1, 0($sp)
        addi $sp, $sp, 1

        lw $a0, 0($sp)
        addi $sp, $sp, 1

        di

        lw $k0, 0($sp)
        addi $sp, $sp, 1

        reti



initsp: .fill 0xA000
ticks:  .fill 0xFFFF
range:  .fill 0xFFFE
maxval: .fill 0xFFFD
minval: .fill 0xFFFC