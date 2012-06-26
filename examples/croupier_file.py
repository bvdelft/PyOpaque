#!/usr/bin/python

from opaque import applyPolicy
from random import shuffle

class Croupier(object):
	def __init__(self):
		self.deck=range(52)
		shuffle(self.deck)
	def drawCard(self):
		return self.deck.pop()

Croupier=applyPolicy(Croupier,'croupier_policy.cfg')
