#include "read.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <exception>

#include "card.h"
#include "cards.h"
#include "deck.h"

void load_decks(Decks& decks, Cards& cards)
{
    if(boost::filesystem::exists("custom.txt")) {
        read_custom_decks(decks, cards, "custom.txt");
    }
}

std::vector<std::pair<std::string, long double>> parse_deck_list(std::string list_string)
{
    std::vector<std::pair<std::string, long double>> res;
    boost::tokenizer<boost::char_delimiters_separator<char>> list_tokens{list_string, boost::char_delimiters_separator<char>{false, ";", ""}};
    for(const auto list_token: list_tokens) {
        boost::tokenizer<boost::char_delimiters_separator<char>> deck_tokens{list_token, boost::char_delimiters_separator<char>{false, ":", ""}};
        auto deck_token = deck_tokens.begin();
        res.push_back(std::make_pair(*deck_token, 1.0d));
        ++deck_token;
        if(deck_token != deck_tokens.end()) {
            res.back().second = boost::lexical_cast<long double>(*deck_token);
        }
    }
    return res;
}

template<typename Iterator, typename Functor>
Iterator advance_until(Iterator it, Iterator it_end, Functor f)
{
    while(it != it_end && !f(*it)) {
        ++it;
    }
    return it;
}

// take care that "it" is 1 past current.
template<typename Iterator, typename Functor>
Iterator recede_until(Iterator it, Iterator it_beg, Functor f)
{
    if (it == it_beg) {
        return it_beg;
    }
    --it;
    do {
        if(f(*it)) {
            return ++it;
        }
        --it;
    } while (it != it_beg);
    return it_beg;
}

template<typename Iterator, typename Functor, typename Token>
Iterator read_token(Iterator it, Iterator it_end, Functor f, Token& token)
{
    Iterator token_start = advance_until(it, it_end, [](const char& c){ return c != ' '; });
    Iterator token_end_after_spaces = advance_until(token_start, it_end, f);
    if (token_start != token_end_after_spaces) {
        Iterator token_end = recede_until(token_end_after_spaces, token_start, [](const char& c){return(c != ' ');});
        token = boost::lexical_cast<Token>(std::string{token_start, token_end});
    }
    return token_end_after_spaces;
}

void parse_card_spec(const Cards& cards, std::string& card_spec, unsigned& card_id, unsigned& card_num, char& num_sign, char& mark)
{
    /* TODO: Can be simplified. */
    auto card_spec_iter = card_spec.begin();
    card_id = 0;
    card_num = 1;
    num_sign = 0;
    mark = 0;
    std::string card_name;
    card_spec_iter = read_token(card_spec_iter, card_spec.end(), [](char c){return(c=='#' || c=='(' || c=='\r');}, card_name);
    if (card_name[0] == '!') {
        mark = card_name[0];
        card_name.erase(0, 1);
    }
    if (card_name.empty()) {
        throw std::runtime_error("No card name");
    }
    // If card name is not found, try find card id quoted in '[]' in name, ignoring other characters.
    std::string simple_name{simplify_name(card_name)};
    auto card_it = cards.player_cards_by_name.find({simple_name, 0});
    if (card_it == cards.player_cards_by_name.end()) {
        card_it = cards.player_cards_by_name.find({simple_name, 1});
    }
    auto card_id_iter = advance_until(simple_name.begin(), simple_name.end(), [](char c){return(c=='[');});
    if (card_it != cards.player_cards_by_name.end()){
        card_id = card_it->second->m_id;
    } else if (card_id_iter != simple_name.end()) {
        ++ card_id_iter;
        card_id_iter = read_token(card_id_iter, simple_name.end(), [](char c){return(c==']');}, card_id);
    }
    if (card_spec_iter != card_spec.end() && (*card_spec_iter == '#' || *card_spec_iter == '(')) {
        ++card_spec_iter;
        if (card_spec_iter != card_spec.end() && strchr("+-$", *card_spec_iter)) {
               num_sign = *card_spec_iter;
               ++card_spec_iter;
        }
        card_spec_iter = read_token(card_spec_iter, card_spec.end(), [](char c){return(c < '0' || c > '9');}, card_num);
    }
    if (card_id == 0) {
        throw std::runtime_error("Unknown card: " + card_name);
    }
}


// Error codes:
// 2 -> file not readable
// 3 -> error while parsing file
unsigned read_custom_decks(Decks& decks, Cards& cards, std::string filename)
{
    std::ifstream decks_file(filename);
    if (!decks_file.is_open()) {
        std::cerr << "Error: Custom deck file " << filename << " could not be opened\n";
        return(2);
    }
    unsigned num_line(0);
    decks_file.exceptions(std::ifstream::badbit);
    try {
        std::string deck_string;

        while (getline(decks_file, deck_string)) {
            ++num_line;
            boost::algorithm::trim(deck_string);
            if(deck_string.size() == 0 || strncmp(deck_string.c_str(), "//", 2) == 0) {
                continue;
            }
            std::string deck_name;
            auto deck_string_iter = read_token(deck_string.begin(), deck_string.end(), [](char c){return(strchr(":,", c));}, deck_name);
            if (deck_string_iter == deck_string.end() || deck_name.empty()) {
                std::cerr << "Error in custom deck file " << filename << " at line " << num_line << ", could not read the deck name.\n";
                continue;
            }
            deck_string_iter = advance_until(deck_string_iter + 1, deck_string.end(), [](const char& c){return(c != ' ');});
            auto deck_iter = decks.by_name.find(deck_name);
            if (deck_iter != decks.by_name.end()) {
                std::cerr << "Warning in custom deck file " << filename << " at line " << num_line << ", name conflicts, overrides " << deck_iter->second->short_description() << std::endl;
            }
            decks.decks.push_back(Deck{DeckType::custom_deck, num_line, deck_name});
            Deck* deck = &decks.decks.back();
            deck->set(cards, std::string{deck_string_iter, deck_string.end()});
            decks.by_name[deck_name] = deck;
            std::stringstream alt_name;
            alt_name << decktype_names[deck->decktype] << " #" << deck->id;
            decks.by_name[alt_name.str()] = deck;
        }
    }
    catch (std::exception & e) {
        std::cerr << "Exception while parsing the custom deck file " << filename;
        if (num_line > 0) {
            std::cerr << " at line " << num_line;
        }
        std::cerr << ": " << e.what() << ".\n";
        return 3;
    }
    return 0;
}

void read_owned_cards(Cards& cards, std::map<unsigned, unsigned>& owned_cards, std::map<unsigned, unsigned>& buyable_cards, const char *filename)
{
    std::ifstream owned_file{filename};
    if(!owned_file.good())
    {
        std::cerr << "Warning: Owned cards file '" << filename << "' does not exist.\n";
        return;
    }
    unsigned num_line(0);
    std::string card_spec;
    while (getline(owned_file, card_spec)) {
        boost::algorithm::trim(card_spec);
        ++num_line;
        if(card_spec.size() == 0 || strncmp(card_spec.c_str(), "//", 2) == 0) {
            continue;
        }
        try {
            unsigned card_id{0};
            unsigned card_num{1};
            char num_sign{0};
            char mark{0};
            parse_card_spec(cards, card_spec, card_id, card_num, num_sign, mark);
            assert(mark == 0);
            if (num_sign == 0) {
                owned_cards[card_id] = card_num;
            }
            else if (num_sign == '+') {
                owned_cards[card_id] += card_num;
            }
            else if (num_sign == '-') {
                owned_cards[card_id] = owned_cards[card_id] > card_num ? owned_cards[card_id] - card_num : 0;
            }
            else if (num_sign == '$') {
                buyable_cards[card_id] = card_num;
            }
        }
        catch(std::exception& e)
        {
            std::cerr << "Error in owned cards file " << filename << " at line " << num_line << " while parsing card '" << card_spec << "': " << e.what() << "\n";
        }
    }
}

