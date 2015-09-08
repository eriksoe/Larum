#!/usr/bin/perl -W

########################################
#                                      #
# LARUM ASSEMBLER                      #
#                                      #
########################################

use integer;

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
    "HELLO" => 1
);

%MUST_FLUSH = ( "CALL" => 1);

my @dest = ();
my ($destpos_opcode, $destpos) = (0,0);
my $opcode_acc;
my $shift;

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
    print STDERR "DB| emit: $v at $destpos\n";
    $dest[$destpos++] = $v;
}

sub flush() {
    $dest[$destpos_opcode] = $opcode_acc;
    init_isr();
}

sub init_isr() {
    $opcode_acc = 0;
    $shift = 0;
    $destpos_opcode = $destpos;
    print STDERR "DB| opcode-pos: $destpos_opcode\n";
    emit_parameter(-1); # Placeholder - reserve space for opcode;
}

sub output($) {
    my ($out_fh) = @_;
    delete $dest[--$destpos] if ($shift == 0);
    for (my $i=0; $i<=$#dest; $i++) {
        print STDERR "  $i: $dest[$i]\n";
        my $x = pack("L", $dest[$i]);
        syswrite($out_fh, $x);
    }
}

init_isr();

my $asmfile = $ARGV[0] or die "No asm file name given.\n";
my $imgfile = $asmfile; $imgfile =~ s/\.asm$//; $imgfile .= ".img";

open(my $in_fh, "<", $asmfile) or die "Could not open asm file `$asmfile': $!\n";
while (<$in_fh>) {
    # Trim:
    s/^\s+//; s/\s+$//;

    if (/^(\S+)\s*(.*)$/) {
        my ($mnemonic, $rest) = ($1,$2);
        $mnemonic = uc($mnemonic);
        if (exists $OPCODE_DEFS{$mnemonic}) {
            my $opdef = $OPCODE_DEFS{$mnemonic};
            my ($opcode, $paramtype) = ($opdef->[0], ($opdef->[1] or ''));
            #print "${opcode} ${paramtype}\n";
            if ($paramtype eq 'I') {
                my $v = int($rest);
                print "${opcode} I:${v}\n";
                emit_parameter($v);
            } elsif ($paramtype eq 'L') {
                my $jump_offset = $rest;
                # TODO: support symbolic labels.
                print "${opcode} I:${jump_offset}\n";
                emit_parameter($jump_offset);
            } elsif ($paramtype eq 'B') {
                my $builtin_name = $rest;
                if (exists $BUILTIN_DEFS{$builtin_name}) {
                    my $bi_nr = $BUILTIN_DEFS{$builtin_name};
                    print "${opcode} B:${bi_nr}\n";
                    emit_parameter($bi_nr);
                } else {
                    die "Unknown builtin: $builtin_name\n";
                }
            } else {
                die "Parameter not expected: $rest\n" unless ($rest eq '');
                print "${opcode}\n";
            }
            add_opcode($opcode);
            flush() if (exists $MUST_FLUSH{$mnemonic});
        } else {
            die "Undefined mnemonic: $mnemonic\n";
        }
    }
}
close($in_fh);

flush();

open(my $out_fh, ">", $imgfile) or die "Could not open output file `$imgfile'\n: $!";
output($out_fh);
close($out_fh)
