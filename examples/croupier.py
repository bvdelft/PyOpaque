#!/usr/bin/python

from opaque import opaque,disableDangerousImports
from random import shuffle

disableDangerousImports()

@opaque(private=['deck'])
class Croupier(object):
	def __init__(self):
		self.deck=range(52)
		shuffle(self.deck)
	def drawCard(self):
		return self.deck.pop()

jack=Croupier()
