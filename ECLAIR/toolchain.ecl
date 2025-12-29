# Compilers.
-file_tag+={GCC,"^tools/gcc-arm-none-eabi-8-2018-q4-major/bin/arm-none-eabi-gcc$"}

# Manuals.
-setq=GCC_MANUAL,"tools/gcc-arm-none-eabi-8-2018-q4-major/share/doc/gcc-arm-none-eabi/pdf/gcc/gcc.pdf"
-setq=CPP_MANUAL,"tools/gcc-arm-none-eabi-8-2018-q4-major/share/doc/gcc-arm-none-eabi/pdf/gcc/cpp.pdf"
-setq=LD_MANUAL,"tools/gcc-arm-none-eabi-8-2018-q4-major/share/doc/gcc-arm-none-eabi/pdf/ld.pdf"
-setq=LIBC_MANUAL,"tools/gcc-arm-none-eabi-8-2018-q4-major/share/doc/gcc-arm-none-eabi/pdf/libc.pdf"
-setq=LIBM_MANUAL,"tools/gcc-arm-none-eabi-8-2018-q4-major/share/doc/gcc-arm-none-eabi/pdf/libm.pdf"

-doc_begin="See Sections \"2.1 _Exit—end program execution with no cleanup processing\", \"2.3 abort—abnormal termination of a program\", and \"2.17 exit—end program execution\" of "LIBC_MANUAL"."
-config=STD.exitstat,+behavior={c18, GCC, "specified"}
-doc_end

-doc_begin="See Sections \"2.2 Include Operation\" and \"11.1 Implementation-defined behavior\" of "CPP_MANUAL"."
-config=STD.inclangl,+behavior={c18, GCC, "specified"}
-config=STD.inclfile,+behavior={c18, GCC, "specified"}
-config=STD.inclhead,+behavior={c18, GCC, "specified"}
-doc_end

-doc_begin="See Section \"4.1 Translation\" of "GCC_MANUAL"."
-config=STD.diagidnt,+behavior={c18, GCC, "specified"}
-doc_end

-doc_begin="See Section \"4.4 Characters\" of "GCC_MANUAL"."
-config=STD.bytebits,+behavior={c18, GCC, 8}
-config=STD.charsmap,+behavior={c18, GCC, "specified"}
-config=STD.charsmem,+behavior={c18, GCC, "eascii"}
-config=STD.execvals,+behavior={c18, GCC, "specified"}
-doc_end

-doc_begin="See Section \"4.5 Integers\" of "GCC_MANUAL"."
-config=STD.signdint,+behavior={c18, GCC, "specified"}
-doc_end

-doc_begin="External identifiers have an unlimited number of significant, case-sensitive characters: see Section \"4.3 Identifiers\" of "GCC_MANUAL" and "LD_MANUAL"."
-config=STD.extidsig,+behavior={c18, GCC, "case_sensitive&&63"}
-doc_end

-doc_begin="See Section \"4.15 Architecture\" of "GCC_MANUAL"."
-config=STD.objbytes,+behavior={c18, GCC, "specified"}
-doc_end

-doc_begin="See Section \"4.13 Preprocessing Directives\" of "GCC_MANUAL" and Section \"7 Pragmas\" of "CPP_MANUAL"."
-config=STD.pragmdir,+behavior={c18, GCC, "^(pop_macro\\(.*|push_macro\\(.*)$"}
-config=STD.pragmhdr,+behavior={c18, GCC, "^(pop_macro\\(.*|push_macro\\(.*)$"}
-config=STD.stringfy,+behavior={c18, GCC, "specified"}
-doc_end
