-- Local integration fixtures: accounts 0..10000. Not production credentials.
USE mydb;
INSERT IGNORE INTO account (AccountNo, ID, PASS, LoginState)
WITH digits AS (
    SELECT 0 AS n UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4
    UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9
)
SELECT n, CONCAT('ID_', n), CONCAT('PASS_', n), 0
FROM (
    SELECT a.n + 10*b.n + 100*c.n + 1000*d.n + 10000*e.n AS n
    FROM digits a CROSS JOIN digits b CROSS JOIN digits c CROSS JOIN digits d CROSS JOIN digits e
) numbers WHERE n <= 10000;
INSERT IGNORE INTO player (AccountNo, PlayerId, Nick, HP, Coins, CharType, ServerType, LocationX, LocationY)
SELECT AccountNo, 0, CONCAT('NICK_', AccountNo), 100, 0, MOD(AccountNo, 4), 0,
       100 + MOD(AccountNo * 37, 5900), 100 + MOD(AccountNo * 53, 5900)
FROM account WHERE AccountNo BETWEEN 0 AND 10000;
