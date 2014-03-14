#ifndef READ_H_INCLUDED
#define READ_H_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "library.h"
#include "deck.h"

void parse_card_spec(const Cards& cards, std::string& card_spec, unsigned& card_id, unsigned& card_num, char& num_sign, char& mark);
void load_decks(Decks& decks, Cards& cards);
std::vector<std::pair<std::string, long double>> parse_deck_list(std::string list_string);
unsigned read_custom_decks(Decks& decks, Cards& cards, std::string filename);
void read_owned_cards(Cards& cards, std::map<unsigned, unsigned>& owned_cards, std::map<unsigned, unsigned>& buyable_cards, const char *filename);

#endif
