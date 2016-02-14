/Evaluation on test.*/ {aftereval=1}
/.*%.*/ {if (aftereval) print $0}

