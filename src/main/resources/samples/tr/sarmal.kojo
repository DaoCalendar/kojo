type Sayı = Doğal
def sarmal(adım: Sayı, açı: Sayı) {
    if (adım <= 300) {
        ileri(adım)
        sağ(açı)
        sarmal(adım + 2, açı)
    }
}
sil()

kalemRenginiKur(siyah)
boyamaRenginiKur(yeşil)
//arkaplanıKurYatay(kırmızı, sarı)
kalemKalınlığınıKur(1)
setSpeed(fast)
sarmal(0, 91)
