#include <avr/io.h>
#include "util.h"
#include "common.h"
#include "queue.h"

void uart_rx_isr(void) __attribute__((signal));
void uart_tx_isr(void) __attribute__((signal));
void t2_150(void) __attribute__((signal));
void t2_300(void) __attribute__((signal));

unsigned char current_rx_queue_slot = -1;
unsigned char rx_packet_bytes;
unsigned char srcaddr_keep;
unsigned char ignore_bytes;

unsigned char current_tx_queue_slot = -1;
unsigned char tx_packet_bytes;
unsigned char tx_packet_bytes_max;

void uart_rx_isr(void)
{
	unsigned char c;
	
	c = UDR0;
	
	if(!ignore_bytes)
	{
		if(srcaddr_keep == 0)
		{
			//no packet right now
			srcaddr_keep = c;
		}
		else
		{
			if(current_rx_queue_slot == -1)
			{
				//2nd byte (dest addr)
				if(c == my_addr || my_addr == 0)
				{
					current_rx_queue_slot = queue_alloc_isr();
					
					if(current_rx_queue_slot != -1)
					{
						//if it didn't fail
						rx_packet_bytes = 2;
						packet_queue[current_rx_queue_slot * MAX_PACKET_SIZE + 0] = srcaddr_keep;
						packet_queue[current_rx_queue_slot * MAX_PACKET_SIZE + 1] = c;
					}
					else
					{
						//we ran out of room!
						rx_overruns++;
						ignore_bytes = 1;
					}
				}
				else
				{
					//not interested
					ignore_bytes = 1;
				}
			}
			else
			{
				//we aren't ignoring stuff, we have a packet, we have a queue slot --> normal
				if(rx_packet_bytes < MAX_PACKET_SIZE)
					packet_queue[current_rx_queue_slot * MAX_PACKET_SIZE + rx_packet_bytes++] = c;
				else
					//somebody made a protocol error
					protocol_errors++;
			}
		}
	}
	
	//delay interrupt
	OCR2A = TCNT2 + TICKS_150US;
	OCR2B = TCNT2 + TICKS_300US;
}

void t2_150(void)
{
	//this isr is processed when a packet is done being recieved
	if(current_rx_queue_slot != -1)
	{
		//we were receiving a packet that we cared about
		
		//reuse to store size
		packet_queue_status[current_rx_queue_slot] = rx_packet_bytes;
		
		//it isn't possible to overrun this
		rx_queue[rx_queue_next++] = current_rx_queue_slot;
		
		//we don't dequeue the packet yet, the processing logic does that
	}
	
	current_rx_queue_slot = -1;
	rx_packet_bytes = 0;
	ignore_bytes = 0;
	srcaddr_keep = 0;
}

void t2_300(void)
{
	if(tx_queue_next != 0)
	{
		//something to send
		current_tx_queue_slot = tx_queue[0];
		tx_packet_bytes = 0;
		tx_packet_bytes_max = packet_queue_status[current_tx_queue_slot];
		
		//disable rx interrupt and enable tx interrupt
		UCSR0B = (UCSR0B & ~(_BV(RXCIE0))) | _BV(UDRIE0);
		tx_on();
	}
}

void uart_tx_isr(void)
{
	unsigned char c, i;
	c = packet_queue[current_tx_queue_slot * MAX_PACKET_SIZE + tx_packet_bytes++];
	UDR0 = c;
	
	//there should not be any bytes before our own packet
	while(!(UCSR0A & _BV(RXC0)));
	
	if(UDR0 != c)
	{
		//we failed to transmit our packet
		
		//enable rx interrupt and disable tx interrupt
		UCSR0B = (UCSR0B | _BV(RXCIE0)) & ~(_BV(UDRIE0));
		
		//fixme: do we need?
		//delay interrupt
		OCR2A = TCNT2 + TICKS_150US;
		OCR2B = TCNT2 + TICKS_300US;
		
		tx_off();
		
		return;
	}
	
	//we did transmit our byte so far
	if(tx_packet_bytes == tx_packet_bytes_max)
	{
		//we're done!
		queue_free(current_tx_queue_slot);
		//should be safe even if more stuff has been enqueued (we are atomic here)
		for(i = 0; i < tx_queue_next-1; i++)
			tx_queue[i] = tx_queue[i+1];
		tx_queue_next--;
		
		//enable rx interrupt and disable tx interrupt
		UCSR0B = (UCSR0B | _BV(RXCIE0)) & ~(_BV(UDRIE0));
		
		tx_off();
	}
	
	//delay interrupt
	OCR2A = TCNT2 + TICKS_150US;
	OCR2B = TCNT2 + TICKS_300US;
}