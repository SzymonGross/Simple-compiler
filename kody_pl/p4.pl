liczba n <- 98765
liczba suma <- 0

liczba cyfra <- 0

dopoki n > 0
{
    cyfra <- n%10
    suma <-+ cyfra
    n <-/ 10
}

liczba wynik <- 0

jezeli suma < 10{
    wynik <- 1
}
albojezeli suma < 25{
    wynik <- 2
}
albojezeli suma < 40 {
    wynik <- 3
}
albo{
    wynik <- 4
}

zakoncz wynik
