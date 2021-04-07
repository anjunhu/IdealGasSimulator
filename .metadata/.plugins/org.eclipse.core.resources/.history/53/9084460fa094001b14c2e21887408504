.syntax unified	// without this we get SUBS instruction not supported in Thumb16 mode
.section .text	//AREA PROGRAM, CODE

//.global (EXPORT) is a keyword indicating functions,
// variables, etc. visible to external segment
.global kalmanFilterA_noStats

/**
* int kalmanFilterA_noStats (float* InputArray, float* OutputArray, struct KalmanState* kstate, int length)
*/

kalmanFilterA_noStats:
			PUSH {R4-R7, LR}		// save previous register status onto stack
			VSTMDB.f32 SP!,{S4-S10}

			MOV R5, R0				// local pointer to current element in InputArray
			MOV R6, R1				// local pointer to current element in OutputArray
			MOV R7, R2
			VLDMIA.f32 R7!, {S4-S8} // local copy of kstate
			MOV R4, R3 				// local downcounter

			VMRS R0, FPSCR			// flush out previous errors
			BIC R0, R0, #15
			VMSR FPSCR, R0


loop:		SUBS R4, R4, #1
			BLT return

			VLDR.f32 S10, [R5]		// S10 = current InputArray element

			VADD.f32 S7, S7, S4 	// p = p + q
			VADD.f32 S9, S7, S5 	// p + r
			VDIV.f32 S8, S7, S9 	// k = p / (p + r)
			VSUB.f32 S9, S10, S6 	// measurement - x
			VMLA.f32 S6, S8, S9 	// x = x + k*(measurement - x)
			VMLS.f32 S7, S8, S7 	// p = p - k*p

			VMRS R0, FPSCR
			ANDS R0, R0, #15		// check for exceptions LSL R0, R0, #28
			BNE exception

			VSTR.f32 S6, [R6]		// current OutputArray element = x

			ADD R5, R5, #4
			ADD R6, R6, #4
			B loop

return:
			VSTMDB.f32 R7!, {S4-S8} // update kstate only if everything went well...
			VLDMIA.f32 SP!,{S4-S10}
			POP {R4-R7, PC}

exception:
			VLDMIA.f32 SP!,{S4-S10}
			POP {R4-R7, PC}

.end
