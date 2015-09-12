#!/usr/bin/perl -W

########################################
#                                      #
# LARUM ASSEMBLER                      #
#                                      #
########################################

use integer;

# The magic number for the file is the UTF-8 # encoding of "L̼B",
# where L is for "Larum" and "̼" is a seagull, and "B" is for "boot image".
$IMG_MAGIC = "\x4c\xcc\xbc\x42";

%OPCODE_DEFS = (
    "\@PC"  => [0],
    "NOP"  => [1],
    "JMP"  => [2, "L"],
    "JMPZ" => [3, "L"],
    "JMPN" => [4, "L"],
    "CALL" => [5, "L"],
    "RET"  => [6],
    "DUP"  => [7],
    "DROP" => [8],
    "OVER" => [9],
    "A>"   => [10],
    ">A"   => [11],
    "R>"   => [12],
    ">R"   => [13],
    "+"    => [14],
    "-"    => [15],
    "*"    => [16],
    ">>"   => [17],
    "<<"   => [18],
    "NOT"  => [19],
    "AND"  => [20],
    "XOR"  => [21],
    "OR"   => [22],
    "LIT"  => [23, "I"],
    "ADDR"  => [23, "L"],
    "LIT1" => [24],
    "\@A"   => [25],
    "!A"   => [26],
    "\@A+"  => [27],
    "!A+"  => [28],
    "\@R+"  => [29],
    "!R+"  => [30],
    "BUILTIN" => [31, "B"]
);

%BUILTIN_DEFS = (
    "EXIT" => 0,
    "HELLO" => 1,
    "DUMP_STACK" => 2,
    "WRITE_STDOUT" => 3,
);

%MUST_FLUSH = ( "CALL" => 1);

my @dest = ();
my ($destpos_opcode, $destpos) = (0,0);
my $opcode_acc;
my $shift;
my %labels = ();
my %patch_table = ();

sub add_opcode($) {
    my ($opcode) = @_;
    $opcode_acc |= ($opcode << $shift);
    $shift += 5;
    if ($shift > 32-5) {
        flush();
    }
}

sub emit_parameter($) {
    my ($v) = @_;
    my $addr = $destpos++;
    $dest[$addr] = $v;
    return $addr;
}

sub flush() {
    return if ($shift == 0);
    $dest[$destpos_opcode] = $opcode_acc;
    init_isr();
}

sub reserve_mem($) {
    delete $dest[--$destpos] if ($shift == 0);
    my ($wordcount) = @_;
    for (my $i=0; $i<$wordcount; $i++) {
        emit_parameter(0);
    }
    init_isr();
}

sub init_isr() {
    $opcode_acc = 0;
    $shift = 0;
    $destpos_opcode = $destpos;
    emit_parameter(-1); # Placeholder - reserve space for opcode;
}

sub patch_labels() {
    my @tmp = %labels;
    print "Labels: @tmp\n";
    for my $patch_addr (keys %patch_table) {
        my $label = $patch_table{$patch_addr};
        die "Unknown label: `$label'\n" unless (exists $labels{$label});
        my $jump_addr = $labels{$label};
        print "patching '$label' = $jump_addr at $patch_addr\n";
        $dest[$patch_addr] = $jump_addr;
    }
}

sub output($) {
    my ($fh) = @_;
    delete $dest[--$destpos] if ($shift == 0);

    syswrite($fh, $IMG_MAGIC);
    syswrite($fh, pack("L", -1)); # Placeholder for checksum
    my $chksum = 0;
    for (my $i=0; $i<=$#dest; $i++) {
        my $d = $dest[$i];
        # print STDERR "  $i: $d\n";
        syswrite($fh, pack("L", $d));
        { use integer;
          $chksum += $d;
        }
    }
    seek($fh, 4, 0);
    syswrite($fh, pack("L", $chksum));
}

init_isr();

my $asmfile = $ARGV[0] or die "No asm file name given.\n";
my $imgfile = $asmfile; $imgfile =~ s/\.asm$//; $imgfile .= ".img";

open(my $in_fh, "<", $asmfile) or die "Could not open asm file `$asmfile': $!\n";
while (<$in_fh>) {
    # Trim:
    s/^\s+//; s/\s+$//;
    # Remove comments:
    s/^#.*//;
    s/^([^\"]*?)\s*#.*/$1/;

    if (/^:(\w+)\s*$/) {
        my $lbl = $1;
        die "Duplicate label: $lbl @ $.\n" if (exists $labels{$lbl});
        flush();
        # print "Defining label `$lbl' = $destpos_opcode\n";
        $labels{$lbl} = $destpos_opcode;
    } elsif (/^(\S+)\s*(.*)$/) {
        my ($mnemonic, $rest) = ($1,$2);
        $mnemonic = uc($mnemonic);
        if (exists $OPCODE_DEFS{$mnemonic}) {
            my $opdef = $OPCODE_DEFS{$mnemonic};
            my ($opcode, $paramtype) = ($opdef->[0], ($opdef->[1] or ''));
            #print "${opcode} ${paramtype}\n";
            if ($paramtype eq 'I') {
                my $v = int($rest);
                #print "${opcode} I:${v}\n";
                emit_parameter($v);
            } elsif ($paramtype eq 'L') {
                my $label = $rest;
                my $addr = emit_parameter(-1);
                $patch_table{$addr} = $label;
            } elsif ($paramtype eq 'B') {
                my $builtin_name = $rest;
                if (exists $BUILTIN_DEFS{$builtin_name}) {
                    my $bi_nr = $BUILTIN_DEFS{$builtin_name};
                    #print "${opcode} B:${bi_nr}\n";
                    emit_parameter($bi_nr);
                } else {
                    die "Unknown builtin: $builtin_name\n";
                }
            } else {
                die "Parameter not expected: $rest\n" unless ($rest eq '');
                #print "${opcode}\n";
            }
            add_opcode($opcode);
            flush() if (exists $MUST_FLUSH{$mnemonic});
        } elsif ($mnemonic eq 'VAR') {
            print "VAR: $rest\n";
            if ($rest =~ /\s*(\w+)(\((\d+)\))?/) {
                my $varname = $1;
                my $varsize = ($3 or 1);
                flush();
                $labels{$varname} = $destpos_opcode;
                print "DB| defining label '$varname' to be here at $destpos_opcode\n";
                reserve_mem($varsize);
            }
        } else {
            die "Undefined mnemonic: $mnemonic\n";
        }
    }
}
close($in_fh);

patch_labels();

flush();

open(my $out_fh, ">", $imgfile) or die "Could not open output file `$imgfile'\n: $!";
output($out_fh);
close($out_fh)
