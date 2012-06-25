#!/usr/bin/python

from opaque import opaque
from random import shuffle

@opaque(private=['deck'])
class Croupier(object):
	def __init__(self):
		self.deck=range(52)
		shuffle(self.deck)
	def drawCard(self):
		return self.deck.pop()

