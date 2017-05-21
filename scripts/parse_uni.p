#!/usr/bin/perl
# $Id: parse_uni.p,v 1.1 2008/04/01 14:49:23 seiji Exp seiji $
# this is a part of skf distribution. See license for use conditions.
# parse_uni.p:	parse UnicodeData from unicode.org and generate a
#		normalization table.
#  Input: UnicodeData-5.*.txt from unicode.org
use utf8;

for $i (0..131072) {	# U+0000 - U+1ffff
    $nfdtbl[$i] = 0;
    $nfctbl[$i] = 0;
};
for $i (0..65536) {
    $nfdcnv[$i] = 0;
    $nstrtbl[$i] = 255;
};
for $i (0..2048) {	# 2f800 - 2fa20 (CJK compat)
    $nfduptbl[$i] = 0;
    $nfcuptbl[$i] = 0;
};

$pntloc = 1;
$csource = 0;
$out_verbose = 0;

# should have a size to fit normaziled codes
$nkdl_l = 20480 - 1;  
#$norm_l = 18600 - 1;
$norm_l = 20480 - 1;
$norm_v = (544 * 2) - 1;
$attrib_l = 65536 - 1;     

if ($ARGV[0] eq "-v") {
    $out_verbose = 1;
    shift @ARGV;
};

if ($ARGV[0] eq "-t") {
    $csource = 1;
    shift @ARGV;
};

if ($csource == 0) {
    open(OUTLFILE, ">table/iOM45P.stb") or die "Can't open output file: $!\n";
    open(OUCLFILE, ">table/iOM45Q.stb") or die "Can't open output file: $!\n";
    open(OUTMFILE, ">table/iOM45R.stb") or die "Can't open output file: $!\n";
    open(OUSTFILE, ">table/iOM45V.stb") or die "Can't open output file: $!\n";
    open(OUTSFILE, ">unicode_compose.c") or die "Can't open output file: $!\n";

# generate skf_table header
    printf OUTLFILE "skf_ctable%c%c",0x32,0x30;
    printf OUCLFILE "skf_ctable%c%c",0x32,0x30;
    printf OUTMFILE "skf_ctable%c%c",0x34,0x30;
    printf OUSTFILE "skf_ctable%c%c",0x32,0x30;
} else {
    open(OUTSFILE, ">unicode_compose.c") or die "Can't open output file: $!\n";
};

while (<>) {
    chomp;	
    $a0 = index($_, ';');
    $b0 = substr($_, 0, $a0);
    $c0 = substr($_, $a0 + 1);
    $a1 = index($c0, ';');
    $b1 = substr($c0, 0, $a1);
    $c1 = substr($c0, $a1 + 1);
    $a2 = index($c1, ';');
    $b2 = substr($c1, 0, $a2);
    $c2 = substr($c1, $a2 + 1);
    $a3 = index($c2, ';');
    $b3 = substr($c2, 0, $a3);
    $c3 = substr($c2, $a3 + 1);
    $a4 = index($c3, ';');
    $b4 = substr($c3, 0, $a4);
    $c4 = substr($c3, $a4 + 1);
    $a5 = index($c4, ';');
    $b5 = substr($c4, 0, $a5);
    $c5 = substr($c4, $a5 + 1);
    $a6 = index($c5, ';');
    $b6 = substr($c5, 0, $a6);
    $c6 = substr($c5, $a6 + 1);
    $a7 = index($c6, ';');
    $b7 = substr($c6, 0, $a7);
    $c7 = substr($c6, $a7 + 1);
    $a8 = index($c7, ';');
    $b8 = substr($c7, 0, $a8);
    $c8 = substr($c7, $a8 + 1);
    $a9 = index($c8, ';');
    $b9 = substr($c8, 0, $a9);
    $c9 = substr($c8, $a9 + 1);
    $aa = index($c9, ';');
    $ba = substr($c9, 0, $aa);
    $ca = substr($c9, $aa + 1);
    $ab = index($ca, ';');
    $bb = substr($ca, 0, $ab);
    $cb = substr($ca, $ab + 1);
    $ac = index($cb, ';');
    $bc = substr($cb, 0, $ac);
    $cc = substr($cb, $ac + 1);
    $ad = index($cc, ';');
    $bd = substr($cc, 1, $ad);
    $cd = substr($cc, $ad + 1);
    $ae = index($cd, ';');
    $be = substr($cd, 1, $ae);
    $ce = substr($cd, $ae + 1);

    $j = hex($b0);	# current character
    if ($b3 eq "Mn") {
    	if ($out_verbose == 1) {
	    printf "Mn:%s - (%s)%d\n", $b0, $b3, $b4;
	};
    	$d4 = $b4 + 0;		# ensure $d4 is integer
	if ($j < 65536)  {
	    $nstrtbl[$j] = $d4;
	};
    };
    if (length($b5) != 0) {	# has substitutes
      if ($j<131072) { # U+0000 - U+1ffff 
	@c5 = split / /,$b5;
	if ((substr($b5,0,1)) ne "<") {	 # pure NKD
	    if ($out_verbose == 1) {
		printf "NKD:%s(%d)%s- ", $b0, $pntloc-1, $b2;
	    };
	    $nfdtbl[$j] = $pntloc;
	} else {
	    if ($out_verbose == 1) {
		printf "NKC:%s(%d)%s- ", $b0, $pntloc-1, $b2;
	    };
	    shift @c5;
	    $nfctbl[$j] = $pntloc;
	    $nfdtbl[$j] = $pntloc;
	};
	foreach $uu (@c5) {
	    $d5 = hex($uu);
	    if ($out_verbose == 1) { printf "%04x ",$d5; };
	    $nfdcnv[$pntloc-1] = $d5;
	    $pntloc++;
	    if ($pntloc > $norm_l) {
		printf "\nError: Table overflow %s %d ",$uu,$pntloc-1;
		die "nfdcnv table overflow\n";
	    };
	};
	$nfdcnv[($pntloc++)-1] = 0;
	if ($out_verbose == 1) { printf "\n"; };
      } elsif (($j>=194560) && ($j < 195104)) { # 2f800 - 2fa20
	$jj = $j - 194560;
	@c5 = split / /,$b5;

	if ((substr($b5,0,1)) ne "<") {
	    if ($out_verbose == 1) {
		printf "NKD:%s(%d)%s- ", $b0, $pntloc, $b2;
	    };
	    $nfduptbl[$jj] = $pntloc;
	} else {
	    if ($out_verbose == 1) {
		printf "NKC:%s(%d)%s- ", $b0, $pntloc, $b2;
	    };
	    $nfcuptbl[$jj] = $pntloc;
	    $nfduptbl[$jj] = $pntloc;
	    shift @c5;
	};
	foreach $uu (@c5) {
	    $d5 = hex($uu);
	    if ($out_verbose == 1) { printf "%04x ",$d5; };
	    $nfdcnv[$pntloc-1] = $d5;
	    $pntloc++;
	    if ($pntloc > $norm_l) {
		printf "\nError Table %s %d ",$uu,$pntloc-1;
		die "nfdcnv table overflow\n";
	    };
	};
	$nfdcnv[($pntloc++)-1] = 0;
	if ($out_verbose == 1) { printf "\n"; };
      };
    };
};
if ($csource == 0) {
  for ($i=0xa0;$i<0x3400;$i++) {
    printf OUTLFILE "%c%c",
	(($nfdtbl[$i] >> 8) & 0xff),
	($nfdtbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfctbl[$i] >> 8) & 0xff),
	($nfctbl[$i] & 0xff);
  };
  for ($i=0xf900;$i<0x10000;$i++) {
    printf OUTLFILE "%c%c",
	(($nfdtbl[$i] >> 8) & 0xff),
	($nfdtbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfctbl[$i] >> 8) & 0xff),
	($nfctbl[$i] & 0xff);
  };
  for ($i=0x1d100;$i<0x1d800;$i++) {
    printf OUTLFILE "%c%c",
	(($nfdtbl[$i] >> 8) & 0xff),
	($nfdtbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfctbl[$i] >> 8) & 0xff),
	($nfctbl[$i] & 0xff);
  };
  for ($i=0;$i<0x220;$i++) {	# 0x2f800 - 0x2fa20
    printf OUTLFILE "%c%c",
	(($nfduptbl[$i] >> 8) & 0xff),
	($nfduptbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfcuptbl[$i] >> 8) & 0xff),
	($nfcuptbl[$i] & 0xff);
  };
  for ($i=0x11090;$i<0x11600;$i++) {
    printf OUTLFILE "%c%c",
	(($nfdtbl[$i] >> 8) & 0xff),
	($nfdtbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfctbl[$i] >> 8) & 0xff),
	($nfctbl[$i] & 0xff);
  };
  for ($i=0x1ee00;$i<0x1ef00;$i++) {
    printf OUTLFILE "%c%c",
	(($nfdtbl[$i] >> 8) & 0xff),
	($nfdtbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfctbl[$i] >> 8) & 0xff),
	($nfctbl[$i] & 0xff);
  };
  for ($i=0x1f100;$i<0x1f400;$i++) {
    printf OUTLFILE "%c%c",
	(($nfdtbl[$i] >> 8) & 0xff),
	($nfdtbl[$i] & 0xff);
    printf OUCLFILE "%c%c",
	(($nfctbl[$i] >> 8) & 0xff),
	($nfctbl[$i] & 0xff);
  };
  for ($i=0;$i<=$norm_l;$i++) {
    printf OUTMFILE "%c%c%c%c",
	(($nfdcnv[$i] >> 24) & 0xff),
	(($nfdcnv[$i] >> 16) & 0xff),
	(($nfdcnv[$i] >> 8) & 0xff),
	($nfdcnv[$i] & 0xff);
  };
  for ($i=0;$i<$attrib_l;$i++) {
    printf OUSTFILE "%c%c",(($nstrtbl[$i] >> 8) & 0xff),($nfctbl[$i] & 0xff);
  };
  printf OUTSFILE "#define uni_nkdl_uni_byte NULL\n";
  printf OUTSFILE "#define uni_nkcl_uni_byte NULL\n";
  printf OUTSFILE "#define uni_norm_uni_byte NULL\n";
  printf OUTSFILE "#define uni_nstr_uni_byte NULL\n";

  printf "\nparse.p: lowtbl - %d uptbl - %d\n",$pntloc, $uppntloc;
} else {
  printf OUTSFILE "unsigned short uni_nkdl_uni_byte[UNI_NKDL_TBL_LEN] = {\n ";
  for ($i=0xa0;$i<0x3400;$i++) {
  	printf OUTSFILE "0x%04x,",$nfdtbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* COMPAT */\n ";
  for ($i=0xf900;$i<0x10000;$i++) {
  	printf OUTSFILE "0x%04x,",$nfdtbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* NOTE */\n ";
  for ($i=0x1d100;$i<0x1d800;$i++) {
  	printf OUTSFILE "0x%04x,",$nfdtbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* CJK-C */\n ";
  for ($i=0;$i<0x220;$i++) {
	printf OUTSFILE "0x%04x,",$nfduptbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* KAITHI */\n ";
  for ($i=0x11090;$i<0x11600;$i++) {
  	printf OUTSFILE "0x%04x,",$nfdtbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* ARAM */\n ";
  for ($i=0x1ee00;$i<0x1ef00;$i++) {
  	printf OUTSFILE "0x%04x,",$nfdtbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* ECSP */\n ";
  for ($i=0x1f100;$i<0x1f400;$i++) {
  	printf OUTSFILE "0x%04x,",$nfdtbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* TAIL */0\n";
  printf OUTSFILE "\n};\n\n";
  printf OUTSFILE "skf_ucode uni_norm_uni_byte[UNI_NORM_TBL_LEN] = {\n ";
  for ($i=0;$i<($norm_l -1);$i++) {
  	printf OUTSFILE "0x%08x,",$nfdcnv[$i];
	if (($i& 3) == 3) { printf OUTSFILE "\n "; };
  };
  printf OUTSFILE "0x%08x\n};\n\n",$nfccnv[$norm_l];
  printf OUTSFILE "unsigned short uni_nkcl_uni_byte[UNI_NKDL_TBL_LEN] = {\n ";
  for ($i=0xa0;$i<0x3400;$i++) {
  	printf OUTSFILE "0x%04x,",$nfctbl[$i];
	if (($i& 7) == 7) { printf OUTSFILE "\n "; };
  };
  printf OUTSFILE "/* ** */\n ";
  for ($i=0xf900;$i<0x10000;$i++) {
  	printf OUTSFILE "0x%04x,",$nfctbl[$i];
	if (($i& 7) == 7) { printf OUTSFILE "\n "; };
  };
  printf OUTSFILE "/* ** */\n ";
  for ($i=0x1d100;$i<0x1d800;$i++) {
  	printf OUTSFILE "0x%04x,",$nfctbl[$i];
	if (($i& 3) == 3) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* CJK-C */\n ";
  for ($i=0;$i<0x220;$i++) {
	printf OUTSFILE "0x%04x,",$nfcuptbl[$i];
	if (($i& 3) == 3) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* KAITHI */\n ";
  for ($i=0x11090;$i<0x11600;$i++) {
  	printf OUTSFILE "0x%04x,",$nfctbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* ARAM */\n ";
  for ($i=0x1ee00;$i<0x1ef00;$i++) {
  	printf OUTSFILE "0x%04x,",$nfctbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* ECSP */\n ";
  for ($i=0x1f100;$i<0x1f400;$i++) {
  	printf OUTSFILE "0x%04x,",$nfctbl[$i];
	if (($i& 7) == 7) {
		printf OUTSFILE "\n ";
	};
  };
  printf OUTSFILE "/* TAIL */0\n";
  printf OUTSFILE "};\n";

  printf OUTSFILE "unsigned short uni_nstr_uni_byte[UNI_ATTRIB_TBL_LEN] = {\n ";
  for ($i=0;$i<($attrib_l-1);$i++) {
  	printf OUTSFILE "0x%04x,",$nstrtbl[$i];
	if (($i& 7) == 7) { printf OUTSFILE "\n "; };
  };
  printf OUTSFILE "0x%04x\n};\n",$nstrtbl[$attrib_l];
  printf "\nparse.p: lowtbl - %d uptbl -%d\n",$pntloc, $uppntloc;
};
