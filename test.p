[i,j,a,b,c]

FOR i := 1 TO 10 DO
    FOR j := 1 TO 10 DO
        ;
    ENDFOR
    a := i + j;
ENDFOR

a := 0;
b := 1;
IF (a <= b) THEN
    c := 1;
    b := -2;
ELSE
    c := 2;
    b := -6;
ENDIF
a := 3;

WHILE (a < 5) DO
    a := a + 1;
    b := b * 2;
    IF (b > 100) THEN
        b := 0;
    ENDIF
ENDWHILE

.
