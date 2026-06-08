liczba a <- 15

liczba b <- 12

dopoki a != b {
    jezeli a < b 
    {
        liczba c <- a
        a <- b
        b <- c
    }

    a <- a - b
}

zakoncz a