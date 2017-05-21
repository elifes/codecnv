# skf conversion table
# this table is for input side, and specify a certain code to
# unic*de conversion.
# by S.Kaneko. This file is in public domain.
#
# put skf conversion table into this directory.
#
1. file name (input table)

example: KS-C_5601 case

              iIM44C.stb
               |||||
I: iso-2022----+|||+--- capitailized iso-2022 calling identify character
N: not     ----+|||
P: private ----+||+---- 6: 96-character set ('7' if small calling identify char)
M: Multibyte  --+|+---- 4: 94 character set ('5' if small calling identify char)
S: Single byte -+|
                 |
4: 4byte seq. ---+
3: 3byte seq. ---+

See source(in_code_table.c) about 'iso-2022 calling identify 
character' for non-iso-2022 codeset, for this part requires
code modification anyway. Third character is +1'd iff iso-2022
calling identify charatcter is small letter.

2. file name (output table)

example: KS-C_5601 case

              oIMMKC.stb
               |||||
I: iso-2022----+|||+---- L: latin code set
N: not     ----+|||+---- G: Graphic char set
                |||+---- N: kana code set 
M: Multibyte  --+||+---- A: CJK extension A charset
S: Single byte -+||+---- K: Kanji code set
                 ||+---- H: Hangul code set
M: Multibase ----+|+---- Y: Y code set
S: Singlebase ---+|+---- C: Compatible area charset
		  |+---- B: CJK extension B charset
		  |+---- X: CJK compatible extension charset
		  |+---- S: ascii code set
		  |+---- M: upmisc code set
		  |+---- U: upkana code set
		  +--- capitalized iso-2022 calling identify character

See source(out_code_table.c) about 'iso-2022 calling identify 
character' for non-iso-2022 codeset, for this part requires
code modification anyway. Third character is +1'd iff iso-2022
calling identify charatcter is small letter.

3. format

Byte 0-9: magic "skf_ctable"
Byte 10:  table entry byte-length '2': 2byte(UCS2), '4': 4byte(UCS4)
Byte 11:  Table revision. Currently '0'
Byte 12-: binary - unsigned big-endian UCS2 code sequence to specify
	  consecutive unic*de location for the code set.

