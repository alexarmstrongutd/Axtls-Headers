#!perl

#   !!!!!!!   DO NOT EDIT THIS FILE   !!!!!!!
#   This file is generated by write_buildcustomize.pl.
#   Any changes made here will be lost!

# We are miniperl, building extensions
# Replace the first entry of @INC ("lib") with the list of
# directories we need.
splice(@INC, 0, 1, q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/AutoLoader/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/dist/Carp/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/dist/PathTools ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/dist/PathTools/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/ExtUtils-Install/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/ExtUtils-MakeMaker/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/ExtUtils-Manifest/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/File-Path/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/ext/re ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/dist/Term-ReadLine/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/dist/Exporter/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/ext/File-Find/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/Text-Tabs/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/dist/constant/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/version/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/cpan/Getopt-Long/lib ,
        q /home/temoc/TypeChef-GNUCHeader/perl-5.30.2/lib );
$^O = 'linux';
__END__
