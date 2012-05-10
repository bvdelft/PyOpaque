#!/usr/bin/python

from random import shuffle

class Croupier(object):
	def __init__(self):
		self.deck=range(52)
		shuffle(self.deck)
	def drawcard(self):
		return self.deck.pop()

match=CardDeck()
