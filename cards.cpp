#include "cards.h"

#include <boost/tokenizer.hpp>
#include <map>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <list>

#include "card.h"
#include "tyrant.h"

template<typename T>
std::string to_string(T val)
{
    std::stringstream s;
    s << val;
    return s.str();
}

std::string simplify_name(const std::string& card_name)
{
    std::string simple_name;
    for(auto c : card_name) {
        if(!strchr(";:, \"'-", c)) {
            simple_name += ::tolower(c);
        }
    }
    return simple_name;
}

//------------------------------------------------------------------------------
Cards::~Cards()
{
    for(Card* c: cards) { delete(c); }
}

const Card* Cards::by_id(unsigned id) const
{
    std::map<unsigned, Card*>::const_iterator cardIter{cards_by_id.find(id)};
    if(cardIter == cards_by_id.end())
    {
        throw std::runtime_error("While trying to find the card with id " + to_string(id) + ": no such key in the cards_by_id map.");
    }
    else
    {
        return(cardIter->second);
    }
}
//------------------------------------------------------------------------------
void Cards::organize()
{
    cards_by_id.clear();
    player_cards.clear();
    player_cards_by_name.clear();
    player_commanders.clear();
    player_assaults.clear();
    player_structures.clear();
    player_actions.clear();
    for(Card* card: cards)
    {
        // Remove delimiters from card names
        size_t pos;
        while((pos = card->m_name.find_first_of(";:,")) != std::string::npos)
        {
            card->m_name.erase(pos, 1);
        }
        if(card->m_set == 5002)
        {
            card->m_name += '*';
        }
        cards_by_id[card->m_id] = card;
        // Card available to players
        if(card->m_set != 0)
        {
            player_cards.push_back(card);
            switch(card->m_type)
            {
                case CardType::commander: {
                    player_commanders.push_back(card);
                    break;
                }
                case CardType::assault: {
                    player_assaults.push_back(card);
                    break;
                }
                case CardType::structure: {
                    player_structures.push_back(card);
                    break;
                }
                case CardType::action: {
                    player_actions.push_back(card);
                    break;
                }
                case CardType::num_cardtypes: {
                    throw card->m_type;
                    break;
                }
            }
            std::string simple_name{simplify_name(card->m_name)};
            auto card_itr = player_cards_by_name.find({simple_name, card->m_hidden});
            if(card_itr == player_cards_by_name.end() || card_itr->second->m_id == card->m_replace)
            {
                player_cards_by_name[{simple_name, card->m_hidden}] = card;
            }
        }
    }
    for(Card* card: cards)
    {
        // update proto_id and upgraded_id
        if(card->m_set == 5002)
        {
            std::string proto_name{simplify_name(card->m_name)};
            proto_name.erase(proto_name.size() - 1);  // remove suffix "*"
            Card * proto_card = player_cards_by_name[{proto_name, card->m_hidden}];
            card->m_proto_id = proto_card->m_id;
            proto_card->m_upgraded_id = card->m_id;
        }
    }
}
