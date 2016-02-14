
opt="`head -1 conf-opt`"
systype="`cat systype`"


case "$opt:$systype" in
  idea:*)
    echo idea
    ;;
  pentium:*)
    echo pentium
    ;;
  ppro:*)
    echo ppro
    ;;
  sparc:*)
    echo sparc
    ;;
  auto:*:*:*:genuineintel-?????5*:*)
    echo pentium
    ;;
  auto:*:*:*:genuineintel-*:*)
    echo ppro
    ;;
  auto:*:i386-*:*:*:*)
    echo ppro
    ;;
  *)
    echo sparc
    ;;
esac
