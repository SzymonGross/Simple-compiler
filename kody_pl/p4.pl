stworz liczba n
ustaw n 98765

stworz liczba suma
ustaw suma 0

stworz liczba cyfra
ustaw cyfra 0

dopoki n > 0
begin
    ustaw cyfra n % 10
    ustaw suma suma + cyfra
    ustaw n n / 10
end

stworz liczba wynik
ustaw wynik 0

jezeli suma < 10
begin
    ustaw wynik 1
end
albojezeli suma < 25
begin
    ustaw wynik 2
end
albojezeli suma < 40
begin
    ustaw wynik 3
end
albo
begin
    ustaw wynik 4
end

zakoncz wynik
