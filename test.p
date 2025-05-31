DECLARE INT i;
DECLARE INT j;
DECLARE INT sum := 0;
DECLARE CONST BOOL isEven;

FOR i := 1 TO 4 DO
    FOR j := 4 DOWNTO 1 DO
        sum := i * j;
    ENDFOR
ENDFOR

isEven := ((i * j) % 2) == 0;
IF isEven == True THEN
    PRINT 100;
ELSE
    PRINT -100;
ENDIF

WHILE sum > 0 DO
    PRINT sum;
    sum := sum - 5;
ENDWHILE
PRINT sum;

EXIT
