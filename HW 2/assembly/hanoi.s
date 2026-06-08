!============================================================
! CS 2200 Homework 2 Part 2: Tower of Hanoi
!
! Apart from initializing the stack,
! please do not edit mains functionality. You do not need to
! save the return address before jumping to hanoi in
! main.
!============================================================

main:
    ! Get the address of the stack using the provided label to initialize the stack pointer.
    ! Load the label address into $sp and in the next instruction, use $sp as base register to load the value (0xFFFF) into $sp.

    lea     $sp, stack              ! loads address of the stack into the stack pointer ($sp)
    lw      $sp, 0($sp)             ! loads the value located at $sp (0xFFFF) into $sp
                                
    lea     $fp, stack              ! loads address of the stack into the frame pointer ($fp)
    lw      $fp, 0($fp)             ! loads the value located at $fp (0xFFFF) into $fp

    lea     $at, hanoi              ! loads address of hanoi label into $at

    lea     $a0, testNumDisks2      ! loads address of number into $a0
    lw      $a0, 0($a0)             ! loads value of number into $a0

    jalr    $at, $ra                ! jump to hanoi, set $ra to return addr
    halt                            ! when we return, just halt

hanoi:
    ! Perform post-call portion of the calling convention. 
    ! Make sure to save any registers you will be using!

    addi    $sp, $sp, -1            ! offset($sp - 1), allocating space for $fp
    sw      $fp, 0($sp)             ! at current $sp, store $fp 
    addi    $fp, $sp, 0             ! $sp --> $fp 
                                

    ! Implement the following pseudocode in assembly:
    ! IF ($a0 == 1) {GOTO base}
    ! ELSE {GOTO else}

    addi    $t0, $zero, 1              ! $t0 = 0 + 1
    beq     $a0, $t0, base             ! if a0 == t0 --> base
    
    beq     $zero, $zero, else         ! GOTO else
else:
    ! Perform recursion after decrementing the parameter by 1. 
    ! Remember, $a0 holds the parameter value.
    addi    $a0, $a0, -1                ! param--
    lea     $at, hanoi                  ! addr[hanoi] --> $at
    addi    $sp, $sp, -2                ! allocate (push) two spots on the stack
    
    sw      $ra, -1($fp)                ! push ra to stack
    sw      $a0, -2($fp)                ! push arg0 to stack

    jalr    $at, $ra                    ! recursive call time

    lw      $ra, -1($fp)                ! pop ra from the stack
    addi    $sp, $sp, 2                 ! deallocate (pop) two spots on the

    ! $v0 = 2 * $v0 + 1
    ! RETURN $v0

    add $v0, $v0, $v0                   ! v0 = v0 + v0 = 2v0
    addi $v0, $v0, 1                    ! v0 = v0 + 1

    beq $zero, $zero, teardown          ! GOTO teardown

base:
    addi    $v0, $v0, 1             ! returns 1

teardown:
    ! Perform pre-return portion of the calling convention
    lw      $fp, 0($fp)             ! old fp
    addi    $sp, $sp, 1             ! pop sp from stack

    jalr    $ra, $zero              ! return to caller



stack: .word 0xFFFF                 ! the stack begins here


! Words for testing \/

! 1
testNumDisks1:
    .word 0x0001

! 10
testNumDisks2:
    .word 0x000a

! 20
testNumDisks3:
    .word 0x0014
