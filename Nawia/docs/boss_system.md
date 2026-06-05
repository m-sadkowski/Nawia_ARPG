# System walk z bossami

System walk z bossami jest oparty na danych i zarzadzany przez `BossManager`. Definicje bossow znajduja sie w `assets/data/bosses.json`, a pliki JSON poziomu umieszczaja encje `boss_trigger`, ktore rozpoczynaja walke.

## Glowne komponenty

### BossManager

- Laduje definicje bossow z `assets/data/bosses.json`.
- Preladowuje encje bossa i minionow, gdy loader lokacji tworzy `boss_trigger`.
- Rozpoczyna i konczy walki z bossami, sledzi pokonanych bossow, przyznaje nagrody graczowi i powiadamia system questow.
- Stosuje mnozniki faz, powiadomienia o fazach i opcjonalne efekty blysku ekranu.
- Czysci przywolane miniony po zakonczeniu walki.

### BossArenaTrigger

`BossArenaTrigger` to prostokatny trigger tworzony z poziomu JSON encji:

```json
{
  "location": "Lesna Dolina",
  "type": "boss_trigger",
  "boss_id": "devil_lord",
  "x": 0.0,
  "y": 2.0,
  "width": 10.0,
  "height": 4.0,
  "trigger_radius": 0
}
```

Trigger rozpoczyna skonfigurowana walke z bossem, gdy gracz wejdzie w jego obszar. Nie restartuje juz aktywnej walki, a `BossManager` uniemozliwia ponowne uruchomienie walki z pokonanym bossem.

W Przedsionku Nawii uzywamy `boss_id: "bies"`. To osobna definicja bossa o
nazwie wyswietlanej `Bies`, ale `enemy_type` zostaje `Devil`, wiec korzysta z
tej samej klasy przeciwnika i animacji.

Boss `czarownica` uzywa `enemy_type: "Witch"`. To dystansowy boss placeholder:
strzela malymi fireballami jako piorunami, po trafieniu powala gracza i
przywoluje `WalkingDead`. Finałowy dialog po jej smierci jest osobna paczka
fabularna.

### Blokowanie teleportow

Obecna implementacja nie posiada dynamicznych scian areny. Teleporty sa blokowane podczas aktywnej walki z bossem, wiec gracz nie moze opuscic starcia przez przejscia miedzy lokacjami.

### UI bossa

`UIHandler` renderuje pasek zdrowia bossa podczas aktywnej walki. Pasek wyswietla nazwe bossa, HP, markery faz, nazwe aktualnej fazy i timer walki. Przejscia miedzy fazami moga rowniez wywolac krotki blysk ekranu skonfigurowany w `bosses.json`.

## JSON bossa

Fazy bossa moga zmieniac mnozniki predkosci i obrazen, wyswietlac powiadomienia, wywolyac blysk ekranu i przywolyc minionow:

```json
{
  "id": "devil_lord",
  "name": "Devil Lord",
  "enemy_type": "Devil",
  "max_hp": 200,
  "scale": 0.03,
  "phases": [
    {
      "hp_threshold": 1.0,
      "name": "Faza 1",
      "speed_multiplier": 1.0,
      "damage_multiplier": 1.0,
      "notification": "Devil Lord atakuje!"
    }
  ],
  "rewards": {
    "gold": 500,
    "exp": 1000,
    "items": [1, 2]
  },
  "on_player_death": "end_fight"
}
```

Pozycja bossa nie jest czescia `bosses.json`. Boss spawnuje sie w srodku
`boss_triggera`, na wysokosci gracza, a potem jest dosuwany do navmesha.
