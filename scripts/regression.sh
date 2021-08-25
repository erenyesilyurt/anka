cutechess-cli.exe -tournament gauntlet -concurrency 4 -pgnout games.pgn \
-engine conf=anka_new tc=10+0.1 \
-engine conf=anka_old tc=10+0.1 \
-openings file=books/Balsa_v500.pgn policy=round -repeat -rounds 500 -games 2